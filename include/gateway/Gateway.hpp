#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <ixwebsocket/IXWebSocket.h>

#include "events/Dispatcher.hpp"
#include "models/Event.hpp"
#include "models/Subscription.hpp"

namespace discord {

class Gateway {
  public:
    using EventCallback = std::function<void(const Event &)>;

    static constexpr const char *kAllEvents = "*";

    Gateway();
    ~Gateway();

    Gateway(const Gateway &) = delete;
    Gateway &operator=(const Gateway &) = delete;
    Gateway(Gateway &&) = delete;
    Gateway &operator=(Gateway &&) = delete;

    void connect(const std::string &url, const std::map<std::string, std::string> &headers = {});
    void disconnect();

    bool isRunning() const noexcept;

    SubscriptionId subscribe(std::string eventName, EventCallback callback);
    SubscriptionId subscribeAll(EventCallback callback);
    bool unsubscribe(SubscriptionId subscriptionId);

    void publish(Event event);
    void send(const std::string &payload);

  private:
    void handleMessage(const ix::WebSocketMessagePtr &message);

    mutable std::mutex mWebsocketMutex;
    std::atomic<bool> mRunning{false};
    detail::Dispatcher mDispatcher;
    ix::WebSocket mWebsocket;
};

}
