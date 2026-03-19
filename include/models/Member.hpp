#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "models/Snowflake.hpp"
#include "models/User.hpp"

namespace discord {

struct Event;

struct Member {
    std::optional<User> user;
    std::optional<Snowflake> guildId;
    std::optional<std::string> nick;
    std::optional<std::string> avatar;
    std::vector<Snowflake> roles;
    std::string joinedAt;
    bool deaf = false;
    bool mute = false;
    bool pending = false;
    int flags = 0;

    static Member from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, Member &m) {
    m.deaf = j.value("deaf", false);
    m.mute = j.value("mute", false);
    m.pending = j.value("pending", false);
    m.flags = j.value("flags", 0);
    m.joinedAt = j.value("joined_at", std::string{});

    if (const auto it = j.find("user"); it != j.end() && !it->is_null())
        m.user = it->get<User>();
    if (const auto it = j.find("guild_id"); it != j.end() && !it->is_null())
        m.guildId = it->get<Snowflake>();
    if (const auto it = j.find("nick"); it != j.end() && !it->is_null())
        m.nick = it->get<std::string>();
    if (const auto it = j.find("avatar"); it != j.end() && !it->is_null())
        m.avatar = it->get<std::string>();
    if (const auto it = j.find("roles"); it != j.end() && it->is_array())
        m.roles = it->get<std::vector<Snowflake>>();
}

}
