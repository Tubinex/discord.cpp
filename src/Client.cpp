#include "Client.hpp"

#include "gateway/Event.hpp"

namespace discord {

namespace {

template <typename Model>
SubscriptionId onEvent(Gateway &gateway, const char *eventName,
                       std::function<void(const Model &)> callback) {
    return gateway.subscribe(eventName, [cb = std::move(callback)](const Event &e) {
        try {
            cb(Model::from(e));
        } catch (...) {
        }
    });
}

}

Client::Client(std::string token, ClientType type, int intents)
    : mGateway(token, type, intents), mHttp(std::move(token), type) {}

void Client::connect() { mGateway.connect(); }
void Client::disconnect() { mGateway.disconnect(); }
bool Client::isRunning() const noexcept { return mGateway.isRunning(); }
bool Client::unsubscribe(SubscriptionId id) { return mGateway.unsubscribe(id); }

SubscriptionId Client::onReady(std::function<void(const Ready &)> callback) {
    return onEvent<Ready>(mGateway, events::READY, std::move(callback));
}

SubscriptionId Client::onMessageCreate(std::function<void(const Message &)> callback) {
    return onEvent<Message>(mGateway, events::MESSAGE_CREATE, std::move(callback));
}

SubscriptionId Client::onMessageUpdate(std::function<void(const Message &)> callback) {
    return onEvent<Message>(mGateway, events::MESSAGE_UPDATE, std::move(callback));
}

SubscriptionId Client::onMessageDelete(std::function<void(const MessageDelete &)> callback) {
    return onEvent<MessageDelete>(mGateway, events::MESSAGE_DELETE, std::move(callback));
}

SubscriptionId Client::onChannelCreate(std::function<void(const Channel &)> callback) {
    return onEvent<Channel>(mGateway, events::CHANNEL_CREATE, std::move(callback));
}

SubscriptionId Client::onChannelUpdate(std::function<void(const Channel &)> callback) {
    return onEvent<Channel>(mGateway, events::CHANNEL_UPDATE, std::move(callback));
}

SubscriptionId Client::onChannelDelete(std::function<void(const Channel &)> callback) {
    return onEvent<Channel>(mGateway, events::CHANNEL_DELETE, std::move(callback));
}

SubscriptionId Client::onGuildUpdate(std::function<void(const Guild &)> callback) {
    return onEvent<Guild>(mGateway, events::GUILD_UPDATE, std::move(callback));
}

SubscriptionId Client::onGuildMemberUpdate(std::function<void(const Member &)> callback) {
    return onEvent<Member>(mGateway, events::GUILD_MEMBER_UPDATE, std::move(callback));
}

SubscriptionId Client::onUserUpdate(std::function<void(const User &)> callback) {
    return onEvent<User>(mGateway, events::USER_UPDATE, std::move(callback));
}

}
