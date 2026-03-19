#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <ixwebsocket/IXWebSocket.h>

#include "events/Dispatcher.hpp"
#include "models/ClientType.hpp"
#include "models/Event.hpp"
#include "models/Subscription.hpp"

namespace discord {

class Gateway {
  public:
    using EventCallback = std::function<void(const Event &)>;

    static constexpr const char *kAllEvents = "*";

    explicit Gateway(std::string token = {}, ClientType type = ClientType::Bot, int intents = 513);
    ~Gateway();

    Gateway(const Gateway &) = delete;
    Gateway &operator=(const Gateway &) = delete;
    Gateway(Gateway &&) = delete;
    Gateway &operator=(Gateway &&) = delete;

    void connect();
    void disconnect();
    bool isRunning() const noexcept;

    SubscriptionId subscribe(std::string eventName, EventCallback callback);
    SubscriptionId subscribeAll(EventCallback callback);
    bool unsubscribe(SubscriptionId subscriptionId);

    void publish(Event event);
    void send(const std::string &payload);

    void setUserAgent(std::string userAgent);

    void setClientBuildNumber(int buildNumber);
    void setOsVersion(std::string osVersion);
    void setOsArch(std::string osArch);
    void setDistro(std::string distro);
    void setDisplayServer(std::string displayServer);
    void setWindowManager(std::string windowManager);

    void enableMessageDump(std::string path);
    void flushMessageDump() const;

  private:
    void handleMessage(const ix::WebSocketMessagePtr &message);
    void handleDiscordMessage(const std::string &json);
    void handleHello(int heartbeatInterval);
    void handleDispatch(int sequence, const std::string &eventName, const std::string &d);
    void sendIdentify();
    void sendHeartbeat();
    void heartbeatLoop();
    void scheduleReconnect();
    void reconnectLoop();
    void stopHeartbeat();
    void mergeEventData(const std::string &eventName, const std::string &d);

    std::string mToken;
    std::string mUserAgent;
    ClientType mClientType;
    int mIntents;
    int mClientBuildNumber = 513221;
    std::string mOsVersion;
    std::string mOsArch;
    std::string mDistro;
    std::string mDisplayServer;
    std::string mWindowManager;
    int mLastSequence = -1;
    int mHeartbeatInterval = 0;

    std::atomic<bool> mHeartbeatRunning{false};
    std::mutex mHeartbeatMutex;
    std::condition_variable mHeartbeatCv;
    std::thread mHeartbeatThread;

    std::atomic<bool> mReconnectPending{false};
    std::atomic<bool> mReconnecting{false};
    std::mutex mReconnectMutex;
    std::condition_variable mReconnectCv;
    std::thread mReconnectThread;

    mutable std::mutex mWebsocketMutex;
    std::atomic<bool> mRunning{false};
    detail::Dispatcher mDispatcher;
    ix::WebSocket mWebsocket;

    std::string mDumpPath;
    mutable std::mutex mDumpMutex;
    std::map<std::string, std::map<std::string, std::string>> mEventDump;
};

}
