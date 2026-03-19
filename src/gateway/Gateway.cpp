#include "gateway/Gateway.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string extractJsonStringField(const std::string &payload, const std::string &key) {
    const std::string keyToken = std::string("\"") + key + "\"";
    const std::size_t keyPosition = payload.find(keyToken);
    if (keyPosition == std::string::npos) {
        return {};
    }

    const std::size_t colonPosition = payload.find(':', keyPosition + keyToken.size());
    if (colonPosition == std::string::npos) {
        return {};
    }

    const std::size_t valueStartQuote = payload.find('"', colonPosition + 1);
    if (valueStartQuote == std::string::npos) {
        return {};
    }

    std::size_t valueEndQuote = valueStartQuote + 1;
    while (true) {
        valueEndQuote = payload.find('"', valueEndQuote);
        if (valueEndQuote == std::string::npos) {
            return {};
        }

        if (payload[valueEndQuote - 1] != '\\') {
            break;
        }

        ++valueEndQuote;
    }

    return payload.substr(valueStartQuote + 1, valueEndQuote - valueStartQuote - 1);
}

std::string extractEventName(const std::string &payload) {
    std::string eventName = extractJsonStringField(payload, "t");
    if (!eventName.empty()) {
        return eventName;
    }

    eventName = extractJsonStringField(payload, "event");
    if (!eventName.empty()) {
        return eventName;
    }

    return "raw_message";
}

discord::Event createEvent(std::string eventName, std::string payload) {
    return discord::Event{std::move(eventName), std::move(payload),
                          std::chrono::system_clock::now()};
}

}

namespace discord {

Gateway::Gateway() {
    mDispatcher.start();
    mWebsocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr &message) { handleMessage(message); });
}

Gateway::~Gateway() {
    disconnect();
    mDispatcher.stop();
}

void Gateway::connect(const std::string &url, const std::map<std::string, std::string> &headers) {
    if (url.empty()) {
        throw std::invalid_argument("url must not be empty");
    }

    std::lock_guard lock(mWebsocketMutex);

    if (mRunning) {
        return;
    }

    ix::WebSocketHttpHeaders websocketHeaders;
    for (const auto &[key, value] : headers) {
        websocketHeaders[key] = value;
    }

    mWebsocket.setUrl(url);
    mWebsocket.setExtraHeaders(websocketHeaders);
    mWebsocket.start();
    mRunning = true;
}

void Gateway::disconnect() {
    std::lock_guard lock(mWebsocketMutex);

    if (!mRunning) {
        return;
    }

    mWebsocket.stop();
    mRunning = false;
}

bool Gateway::isRunning() const noexcept { return mRunning.load(); }

SubscriptionId Gateway::subscribe(std::string eventName, EventCallback callback) {
    return mDispatcher.subscribe(std::move(eventName), std::move(callback));
}

SubscriptionId Gateway::subscribeAll(EventCallback callback) {
    return mDispatcher.subscribe(kAllEvents, std::move(callback));
}

bool Gateway::unsubscribe(SubscriptionId subscriptionId) {
    return mDispatcher.unsubscribe(subscriptionId);
}

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

void Gateway::handleMessage(const ix::WebSocketMessagePtr &message) {
    switch (message->type) {
    case ix::WebSocketMessageType::Open:
        mDispatcher.publish(createEvent("socket_open", message->openInfo.uri));
        break;
    case ix::WebSocketMessageType::Close:
        mDispatcher.publish(createEvent("socket_close", message->closeInfo.reason));
        break;
    case ix::WebSocketMessageType::Error:
        mDispatcher.publish(createEvent("socket_error", message->errorInfo.reason));
        break;
    case ix::WebSocketMessageType::Message:
        mDispatcher.publish(createEvent(extractEventName(message->str), message->str));
        break;
    default:
        break;
    }
}

}
