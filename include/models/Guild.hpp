#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "models/Member.hpp"
#include "models/Role.hpp"
#include "models/Snowflake.hpp"

namespace discord {

struct Event;

struct Guild {
    Snowflake id;
    std::string name;
    std::optional<std::string> icon;
    std::optional<std::string> splash;
    std::optional<std::string> banner;
    Snowflake ownerId;
    std::string preferredLocale;
    int memberCount = 0;
    int verificationLevel = 0;
    int premiumTier = 0;
    int premiumSubscriptionCount = 0;
    std::vector<Role> roles;
    std::vector<Member> members;

    static Guild from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, Guild &g) {
    j.at("id").get_to(g.id);
    j.at("name").get_to(g.name);
    g.ownerId = j.value("owner_id", std::string{});
    g.preferredLocale = j.value("preferred_locale", std::string{});
    g.memberCount = j.value("member_count", 0);
    g.verificationLevel = j.value("verification_level", 0);
    g.premiumTier = j.value("premium_tier", 0);
    g.premiumSubscriptionCount = j.value("premium_subscription_count", 0);

    if (const auto it = j.find("icon"); it != j.end() && !it->is_null())
        g.icon = it->get<std::string>();
    if (const auto it = j.find("splash"); it != j.end() && !it->is_null())
        g.splash = it->get<std::string>();
    if (const auto it = j.find("banner"); it != j.end() && !it->is_null())
        g.banner = it->get<std::string>();
    if (const auto it = j.find("roles"); it != j.end() && it->is_array())
        g.roles = it->get<std::vector<Role>>();
    if (const auto it = j.find("members"); it != j.end() && it->is_array())
        g.members = it->get<std::vector<Member>>();
}

}
