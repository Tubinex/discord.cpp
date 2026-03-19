#include "models/User.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

User User::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<User>();
}

}
