#include "models/Ready.hpp"

#include <nlohmann/json.hpp>

#include "models/Event.hpp"

namespace discord {

Ready Ready::from(const discord::Event &event) {
    return nlohmann::json::parse(event.mPayload).get<Ready>();
}

}
