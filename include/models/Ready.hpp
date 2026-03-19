#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "models/User.hpp"

namespace discord {

struct Event;

struct Ready {
    int v = 0;
    User user;
    std::string sessionId;
    std::string resumeGatewayUrl;

    static Ready from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, Ready &r) {
    r.v = j.value("v", 0);
    r.sessionId = j.value("session_id", std::string{});
    r.resumeGatewayUrl = j.value("resume_gateway_url", std::string{});
    if (const auto it = j.find("user"); it != j.end() && !it->is_null())
        r.user = it->get<User>();
}

}
