#include "gateway/Gateway.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::pair<std::string, std::size_t> valueAt(const std::string &json, std::size_t pos) {
    if (pos >= json.size()) {
        return {{}, pos};
    }

    const char first = json[pos];

    if (first == '"') {
        std::size_t end = pos + 1;
        while (end < json.size()) {
            if (json[end] == '\\') {
                end += 2;
                continue;
            }
            if (json[end] == '"') {
                ++end;
                break;
            }
            ++end;
        }
        return {json.substr(pos, end - pos), end};
    }

    if (first == '{' || first == '[') {
        const char open = first;
        const char close = (first == '{') ? '}' : ']';
        int depth = 1;
        bool inStr = false;
        std::size_t cur = pos + 1;
        while (cur < json.size() && depth > 0) {
            if (inStr) {
                if (json[cur] == '\\') {
                    ++cur;
                } else if (json[cur] == '"') {
                    inStr = false;
                }
            } else {
                if (json[cur] == '"') {
                    inStr = true;
                } else if (json[cur] == open) {
                    ++depth;
                } else if (json[cur] == close) {
                    --depth;
                }
            }
            ++cur;
        }
        return {json.substr(pos, cur - pos), cur};
    }

    const std::size_t end = json.find_first_of(",}] \t\r\n", pos);
    const std::size_t actualEnd = (end == std::string::npos) ? json.size() : end;
    return {json.substr(pos, actualEnd - pos), actualEnd};
}

std::map<std::string, std::string> topLevelFields(const std::string &json) {
    std::map<std::string, std::string> result;
    if (json.empty() || json.front() != '{') {
        return result;
    }

    std::size_t pos = 1;
    while (pos < json.size()) {
        pos = json.find_first_not_of(" \t\r\n", pos);
        if (pos == std::string::npos || json[pos] == '}') {
            break;
        }
        if (json[pos] != '"') {
            break;
        }

        const std::size_t keyStart = pos + 1;
        std::size_t keyEnd = keyStart;
        while (keyEnd < json.size()) {
            keyEnd = json.find('"', keyEnd);
            if (keyEnd == std::string::npos) {
                return result;
            }
            if (json[keyEnd - 1] != '\\') {
                break;
            }
            ++keyEnd;
        }
        const std::string key = json.substr(keyStart, keyEnd - keyStart);
        pos = keyEnd + 1;

        pos = json.find_first_not_of(" \t\r\n", pos);
        if (pos == std::string::npos || json[pos] != ':') {
            return result;
        }
        ++pos;

        pos = json.find_first_not_of(" \t\r\n", pos);
        if (pos == std::string::npos) {
            return result;
        }
        auto [value, nextPos] = valueAt(json, pos);
        result[key] = std::move(value);
        pos = nextPos;

        pos = json.find_first_not_of(" \t\r\n", pos);
        if (pos == std::string::npos) {
            break;
        }
        if (json[pos] == ',') {
            ++pos;
        } else if (json[pos] == '}') {
            break;
        }
    }
    return result;
}

std::string jsonString(const std::string &json, const std::string &key) {
    const std::string keyToken = "\"" + key + "\"";
    const std::size_t keyPos = json.find(keyToken);
    if (keyPos == std::string::npos) {
        return {};
    }
    const std::size_t colonPos = json.find(':', keyPos + keyToken.size());
    if (colonPos == std::string::npos) {
        return {};
    }
    const std::size_t afterColon = json.find_first_not_of(" \t\r\n", colonPos + 1);
    if (afterColon == std::string::npos || json[afterColon] != '"') {
        return {};
    }
    std::size_t end = afterColon + 1;
    while (true) {
        end = json.find('"', end);
        if (end == std::string::npos) {
            return {};
        }
        if (json[end - 1] != '\\') {
            break;
        }
        ++end;
    }
    return json.substr(afterColon + 1, end - afterColon - 1);
}

int jsonInt(const std::string &json, const std::string &key) {
    const std::string keyToken = "\"" + key + "\"";
    const std::size_t keyPos = json.find(keyToken);
    if (keyPos == std::string::npos) {
        return -1;
    }
    const std::size_t colonPos = json.find(':', keyPos + keyToken.size());
    if (colonPos == std::string::npos) {
        return -1;
    }
    const std::size_t afterColon = json.find_first_not_of(" \t\r\n", colonPos + 1);
    if (afterColon == std::string::npos) {
        return -1;
    }
    if (json.compare(afterColon, 4, "null") == 0) {
        return -1;
    }
    try {
        return std::stoi(json.substr(afterColon));
    } catch (...) {
        return -1;
    }
}

std::string jsonValue(const std::string &json, const std::string &key) {
    const std::string keyToken = "\"" + key + "\"";
    const std::size_t keyPos = json.find(keyToken);
    if (keyPos == std::string::npos) {
        return {};
    }
    const std::size_t colonPos = json.find(':', keyPos + keyToken.size());
    if (colonPos == std::string::npos) {
        return {};
    }
    const std::size_t valueStart = json.find_first_not_of(" \t\r\n", colonPos + 1);
    if (valueStart == std::string::npos) {
        return {};
    }
    return valueAt(json, valueStart).first;
}

std::string jsonEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (const unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        default:
            if (c < 0x20) {
                // other control characters → \uXXXX
                char buf[7];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
}

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
    std::ofstream file(mDumpPath);
    file << "{\n";
    bool firstEvent = true;
    for (const auto &[eventName, fields] : mEventDump) {
        if (!firstEvent) {
            file << ",\n";
        }
        firstEvent = false;
        file << "  \"" << eventName << "\": {\n";
        bool firstField = true;
        for (const auto &[key, value] : fields) {
            if (!firstField) {
                file << ",\n";
            }
            firstField = false;
            file << "    \"" << key << "\": " << value;
        }
        file << "\n  }";
    }
    file << "\n}\n";
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

void Gateway::handleDiscordMessage(const std::string &json) {
    switch (jsonInt(json, "op")) {
    case 0: // dispatch
        handleDispatch(jsonInt(json, "s"), jsonString(json, "t"), jsonValue(json, "d"));
        break;
    case 1: // requested heartbeat
        sendHeartbeat();
        break;
    case 7: // reconnect
        scheduleReconnect();
        break;
    case 10: // hello
        handleHello(jsonValue(json, "d"));
        break;
    case 11: // heartbeat acknowledgement
        break;
    default:
        break;
    }
}

void Gateway::handleHello(const std::string &d) {
    const int interval = jsonInt(d, "heartbeat_interval");
    if (interval > 0) {
        mHeartbeatInterval = interval;
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

    std::string payload;

    const std::string ua = mUserAgent.empty() ? "discord.cpp" : mUserAgent;
    const std::string token = jsonEscape(mToken);

    if (mClientType == ClientType::Bot) {
        payload = "{\"op\":2,\"d\":{"
                  "\"token\":\"" +
                  token +
                  "\","
                  "\"intents\":" +
                  std::to_string(mIntents) +
                  ","
                  "\"properties\":{"
                  "\"os\":\"linux\","
                  "\"browser\":\"" +
                  jsonEscape(ua) +
                  "\","
                  "\"device\":\"" +
                  jsonEscape(ua) +
                  "\""
                  "}}}";
    } else {
        const std::string osVersion = jsonEscape(mOsVersion);
        const std::string osArch = jsonEscape(mOsArch);
        const std::string distro = jsonEscape(mDistro);
        const std::string displaySrv = jsonEscape(mDisplayServer);
        const std::string wm = jsonEscape(mWindowManager);

        payload = "{\"op\":2,\"d\":{"
                  "\"token\":\"" +
                  token +
                  "\","
                  "\"capabilities\":1734653,"
                  "\"properties\":{"
                  "\"os\":\"Linux\","
                  "\"browser\":\"Discord Client\","
                  "\"release_channel\":\"stable\","
                  "\"client_version\":\"0.0.75\","
                  "\"os_version\":\"" +
                  osVersion +
                  "\","
                  "\"os_arch\":\"" +
                  osArch +
                  "\","
                  "\"app_arch\":\"" +
                  osArch +
                  "\","
                  "\"system_locale\":\"en-US\","
                  "\"browser_user_agent\":\"" +
                  jsonEscape(ua) +
                  "\","
                  "\"browser_version\":\"37.6.0\","
                  "\"client_build_number\":" +
                  std::to_string(mClientBuildNumber) +
                  ","
                  "\"native_build_number\":null,"
                  "\"client_event_source\":null,"
                  "\"design_id\":0,"
                  "\"has_client_mods\":false,"
                  "\"client_launch_id\":\"\","
                  "\"window_manager\":\"" +
                  wm +
                  "\","
                  "\"distro\":\"" +
                  distro +
                  "\","
                  "\"runtime_environment\":\"native\","
                  "\"display_server\":\"" +
                  displaySrv +
                  "\""
                  "},"
                  "\"presence\":{"
                  "\"status\":\"unknown\","
                  "\"since\":0,"
                  "\"activities\":[],"
                  "\"afk\":false"
                  "},"
                  "\"compress\":false,"
                  "\"client_state\":{"
                  "\"guild_versions\":{},"
                  "\"highest_last_message_id\":\"0\","
                  "\"read_state_version\":0,"
                  "\"user_guild_settings_version\":-1,"
                  "\"user_settings_hash\":\"\","
                  "\"partial_guild_subscriptions\":false,"
                  "\"api_code_version\":0"
                  "},"
                  "\"is_fast_connect\":true"
                  "}}";
    }

    std::lock_guard lock(mWebsocketMutex);
    if (mRunning) {
        mWebsocket.send(payload);
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
    const auto fields = topLevelFields(d);
    if (fields.empty()) {
        return;
    }
    std::lock_guard lock(mDumpMutex);
    auto &stored = mEventDump[eventName];
    for (const auto &[key, value] : fields) {
        stored[key] = value;
    }
}

}
