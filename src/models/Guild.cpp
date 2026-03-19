#include "models/Guild.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

Guild Guild::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<Guild>();
}

}
