#include "models/Message.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

Message Message::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<Message>();
}

MessageDelete MessageDelete::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<MessageDelete>();
}

}
