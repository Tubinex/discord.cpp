#include "models/Channel.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

Channel Channel::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<Channel>();
}

TextChannel TextChannel::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<TextChannel>();
}

VoiceChannel VoiceChannel::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<VoiceChannel>();
}

}
