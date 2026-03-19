#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "models/Snowflake.hpp"

namespace discord {

struct Role {
    Snowflake id;
    std::string name;
    int color = 0;
    bool hoist = false;
    int position = 0;
    std::string permissions;
    bool managed = false;
    bool mentionable = false;
    int flags = 0;
};

inline void from_json(const nlohmann::json &j, Role &r) {
    j.at("id").get_to(r.id);
    j.at("name").get_to(r.name);
    r.color = j.value("color", 0);
    r.hoist = j.value("hoist", false);
    r.position = j.value("position", 0);
    r.permissions = j.value("permissions", std::string("0"));
    r.managed = j.value("managed", false);
    r.mentionable = j.value("mentionable", false);
    r.flags = j.value("flags", 0);
}

}
