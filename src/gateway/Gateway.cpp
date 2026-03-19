#include "gateway/Gateway.hpp"

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

discord::Event makeEvent(std::string name, std::string payload) {
    return discord::Event{std::move(name), std::move(payload), std::chrono::system_clock::now()};
}

}

namespace discord {

Gateway::Gateway(std::string token, ClientType type, int intents)
    : mToken(std::move(token)), mClientType(type), mIntents(intents) {
    mDispatcher.start();
    mWebsocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr &msg) { handleMessage(msg); });
    mReconnectThread = std::thread([this] { reconnectLoop(); });
}

Gateway::~Gateway() {
    disconnect();
    if (!mDumpPath.empty()) {
        flushMessageDump();
    }
    mDispatcher.stop();
}

void Gateway::connect() {
    std::lock_guard lock(mWebsocketMutex);
    if (mRunning) {
        return;
    }
    mWebsocket.setUrl("wss://gateway.discord.gg/?v=10&encoding=json");
    mWebsocket.start();
    mRunning = true;
}

void Gateway::disconnect() {
    const bool wasRunning = mRunning.exchange(false);

    mReconnectCv.notify_all();
    if (mReconnectThread.joinable()) {
        mReconnectThread.join();
    }

    stopHeartbeat();

    if (wasRunning) {
        mWebsocket.stop();
    }
}

bool Gateway::isRunning() const noexcept { return mRunning.load(); }

SubscriptionId Gateway::subscribe(std::string eventName, EventCallback callback) {
    return mDispatcher.subscribe(std::move(eventName), std::move(callback));
}

SubscriptionId Gateway::subscribeAll(EventCallback callback) {
    return mDispatcher.subscribe(kAllEvents, std::move(callback));
}

bool Gateway::unsubscribe(SubscriptionId id) { return mDispatcher.unsubscribe(id); }

void Gateway::publish(Event event) {
    if (event.mReceivedAt.time_since_epoch().count() == 0) {
        event.mReceivedAt = std::chrono::system_clock::now();
    }
    mDispatcher.publish(std::move(event));
}

void Gateway::send(const std::string &payload) {
    std::lock_guard lock(mWebsocketMutex);
    if (!mRunning) {
        throw std::runtime_error("websocket connection is not running");
    }
    mWebsocket.send(payload);
}

void Gateway::setUserAgent(std::string userAgent) { mUserAgent = std::move(userAgent); }

void Gateway::setClientBuildNumber(int buildNumber) { mClientBuildNumber = buildNumber; }
void Gateway::setOsVersion(std::string v) { mOsVersion = std::move(v); }
void Gateway::setOsArch(std::string v) { mOsArch = std::move(v); }
void Gateway::setDistro(std::string v) { mDistro = std::move(v); }
void Gateway::setDisplayServer(std::string v) { mDisplayServer = std::move(v); }
void Gateway::setWindowManager(std::string v) { mWindowManager = std::move(v); }

void Gateway::enableMessageDump(std::string path) {
    std::lock_guard lock(mDumpMutex);
    mDumpPath = std::move(path);
}

void Gateway::flushMessageDump() const {
    std::lock_guard lock(mDumpMutex);
    if (mDumpPath.empty() || mEventDump.empty()) {
        return;
    }
    nlohmann::json out = nlohmann::json::object();
    for (const auto &[eventName, fields] : mEventDump) {
        auto &eventObj = out[eventName];
        eventObj = nlohmann::json::object();
        for (const auto &[key, value] : fields) {
            try {
                eventObj[key] = nlohmann::json::parse(value);
            } catch (...) {
                eventObj[key] = value;
            }
        }
    }
    std::ofstream file(mDumpPath);
    file << out.dump(2) << '\n';
}

void Gateway::handleMessage(const ix::WebSocketMessagePtr &message) {
    switch (message->type) {
    case ix::WebSocketMessageType::Open:
        mDispatcher.publish(makeEvent("socket_open", message->openInfo.uri));
        break;
    case ix::WebSocketMessageType::Close:
        mDispatcher.publish(makeEvent("socket_close", std::to_string(message->closeInfo.code) +
                                                          " " + message->closeInfo.reason));
        if (mRunning && !mReconnecting) {
            scheduleReconnect();
        }
        break;
    case ix::WebSocketMessageType::Error:
        mDispatcher.publish(makeEvent("socket_error", message->errorInfo.reason));
        break;
    case ix::WebSocketMessageType::Message:
        handleDiscordMessage(message->str);
        break;
    default:
        break;
    }
}

void Gateway::handleDiscordMessage(const std::string &raw) {
    try {
        const auto j = nlohmann::json::parse(raw);
        const int op = j.value("op", -1);
        switch (op) {
        case 0:
            handleDispatch(j.value("s", -1), j.value("t", std::string{}),
                           j.contains("d") && !j["d"].is_null() ? j["d"].dump() : "{}");
            break;
        case 1:
            sendHeartbeat();
            break;
        case 7:
            scheduleReconnect();
            break;
        case 10:
            handleHello(j.contains("d") ? j["d"].value("heartbeat_interval", 45000) : 45000);
            break;
        case 11:
            break;
        default:
            break;
        }
    } catch (...) {
    }
}

void Gateway::handleHello(int heartbeatInterval) {
    if (heartbeatInterval > 0) {
        mHeartbeatInterval = heartbeatInterval;
    }

    sendIdentify();
    stopHeartbeat();
    mHeartbeatRunning = true;
    mHeartbeatThread = std::thread([this] { heartbeatLoop(); });
}

void Gateway::handleDispatch(int sequence, const std::string &eventName, const std::string &d) {
    if (sequence >= 0) {
        mLastSequence = sequence;
    }
    if (eventName.empty()) {
        return;
    }
    if (!mDumpPath.empty()) {
        mergeEventData(eventName, d);
    }
    mDispatcher.publish(makeEvent(eventName, d));
}

void Gateway::sendIdentify() {
    if (mToken.empty()) {
        return;
    }

    const std::string ua = mUserAgent.empty() ? "discord.cpp" : mUserAgent;

    nlohmann::json j;
    j["op"] = 2;

    if (mClientType == ClientType::Bot) {
        j["d"] = {{"token", mToken},
                  {"intents", mIntents},
                  {"properties", {{"os", "linux"}, {"browser", ua}, {"device", ua}}}};
    } else {
        j["d"] = {{"token", mToken},
                  {"capabilities", 1734653},
                  {"properties",
                   {{"os", "Linux"},
                    {"browser", "Discord Client"},
                    {"release_channel", "stable"},
                    {"client_version", "0.0.75"},
                    {"os_version", mOsVersion},
                    {"os_arch", mOsArch},
                    {"app_arch", mOsArch},
                    {"system_locale", "en-US"},
                    {"browser_user_agent", ua},
                    {"browser_version", "37.6.0"},
                    {"client_build_number", mClientBuildNumber},
                    {"native_build_number", nullptr},
                    {"client_event_source", nullptr},
                    {"design_id", 0},
                    {"has_client_mods", false},
                    {"client_launch_id", ""},
                    {"window_manager", mWindowManager},
                    {"distro", mDistro},
                    {"runtime_environment", "native"},
                    {"display_server", mDisplayServer}}},
                  {"presence",
                   {{"status", "unknown"},
                    {"since", 0},
                    {"activities", nlohmann::json::array()},
                    {"afk", false}}},
                  {"compress", false},
                  {"client_state",
                   {{"guild_versions", nlohmann::json::object()},
                    {"highest_last_message_id", "0"},
                    {"read_state_version", 0},
                    {"user_guild_settings_version", -1},
                    {"user_settings_hash", ""},
                    {"partial_guild_subscriptions", false},
                    {"api_code_version", 0}}},
                  {"is_fast_connect", true}};
    }

    std::lock_guard lock(mWebsocketMutex);
    if (mRunning) {
        mWebsocket.send(j.dump());
    }
}

void Gateway::sendHeartbeat() {
    const std::string payload = mLastSequence >= 0
                                    ? "{\"op\":1,\"d\":" + std::to_string(mLastSequence) + "}"
                                    : "{\"op\":1,\"d\":null}";
    std::lock_guard lock(mWebsocketMutex);
    if (mRunning) {
        mWebsocket.send(payload);
    }
}

void Gateway::heartbeatLoop() {
    while (mHeartbeatRunning) {
        sendHeartbeat();

        std::unique_lock lock(mHeartbeatMutex);
        mHeartbeatCv.wait_for(lock, std::chrono::milliseconds(mHeartbeatInterval),
                              [this] { return !mHeartbeatRunning.load(); });
        if (!mHeartbeatRunning) {
            break;
        }
    }
}

void Gateway::stopHeartbeat() {
    {
        std::lock_guard lock(mHeartbeatMutex);
        mHeartbeatRunning = false;
    }
    mHeartbeatCv.notify_all();
    if (mHeartbeatThread.joinable()) {
        mHeartbeatThread.join();
    }
}

void Gateway::scheduleReconnect() {
    {
        std::lock_guard lock(mReconnectMutex);
        mReconnectPending = true;
    }
    mReconnectCv.notify_one();
}

void Gateway::reconnectLoop() {
    while (true) {
        std::unique_lock lock(mReconnectMutex);
        mReconnectCv.wait(lock, [this] { return mReconnectPending.load() || !mRunning.load(); });

        if (!mRunning) {
            break;
        }

        mReconnectPending = false;
        mReconnecting = true;
        lock.unlock();

        stopHeartbeat();
        mWebsocket.stop();
        if (mRunning) {
            mWebsocket.start();
        }

        mReconnecting = false;
    }
}

void Gateway::mergeEventData(const std::string &eventName, const std::string &d) {
    try {
        const auto j = nlohmann::json::parse(d);
        if (!j.is_object()) {
            return;
        }
        std::lock_guard lock(mDumpMutex);
        auto &stored = mEventDump[eventName];
        for (const auto &[key, value] : j.items()) {
            stored[key] = value.dump();
        }
    } catch (...) {
    }
}

}
