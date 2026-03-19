#include "models/Member.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

Member Member::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<Member>();
}

}
