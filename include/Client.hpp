#pragma once

#include <functional>
#include <string>

#include "gateway/Gateway.hpp"
#include "http/Client.hpp"
#include "models/Channel.hpp"
#include "models/ClientType.hpp"
#include "models/Guild.hpp"
#include "models/Member.hpp"
#include "models/Message.hpp"
#include "models/Ready.hpp"
#include "models/Subscription.hpp"
#include "models/User.hpp"

namespace discord {

class Client {
  public:
    explicit Client(std::string token, ClientType type = ClientType::Bot, int intents = 513);

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(Client &&) = delete;

    void connect();
    void disconnect();
    bool isRunning() const noexcept;

    SubscriptionId onReady(std::function<void(const Ready &)> callback);
    SubscriptionId onMessageCreate(std::function<void(const Message &)> callback);
    SubscriptionId onMessageUpdate(std::function<void(const Message &)> callback);
    SubscriptionId onMessageDelete(std::function<void(const MessageDelete &)> callback);
    SubscriptionId onChannelCreate(std::function<void(const Channel &)> callback);
    SubscriptionId onChannelUpdate(std::function<void(const Channel &)> callback);
    SubscriptionId onChannelDelete(std::function<void(const Channel &)> callback);
    SubscriptionId onGuildUpdate(std::function<void(const Guild &)> callback);
    SubscriptionId onGuildMemberUpdate(std::function<void(const Member &)> callback);
    SubscriptionId onUserUpdate(std::function<void(const User &)> callback);

    bool unsubscribe(SubscriptionId id);

    Gateway &gateway() noexcept { return mGateway; }
    HttpClient &http() noexcept { return mHttp; }

  private:
    template <typename Model>
    SubscriptionId on(const char *eventName, std::function<void(const Model &)> callback);

    Gateway mGateway;
    HttpClient mHttp;
};

}
