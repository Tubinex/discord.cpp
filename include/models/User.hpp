#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "models/Snowflake.hpp"

namespace discord {

struct Event;

struct User {
    Snowflake id;
    std::string username;
    std::string discriminator;
    std::optional<std::string> globalName;
    std::optional<std::string> avatar;
    std::optional<std::string> banner;
    bool bot = false;
    bool system = false;
    int publicFlags = 0;

    static User from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, User &u) {
    j.at("id").get_to(u.id);
    j.at("username").get_to(u.username);
    u.discriminator = j.value("discriminator", std::string("0"));
    u.bot = j.value("bot", false);
    u.system = j.value("system", false);
    u.publicFlags = j.value("public_flags", 0);

    if (const auto it = j.find("global_name"); it != j.end() && !it->is_null())
        u.globalName = it->get<std::string>();
    if (const auto it = j.find("avatar"); it != j.end() && !it->is_null())
        u.avatar = it->get<std::string>();
    if (const auto it = j.find("banner"); it != j.end() && !it->is_null())
        u.banner = it->get<std::string>();
}

}
