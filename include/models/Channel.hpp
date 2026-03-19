#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "models/Snowflake.hpp"

namespace discord {

struct Event;

enum class ChannelType : std::uint8_t {
    GuildText = 0,
    Dm = 1,
    GuildVoice = 2,
    GroupDm = 3,
    GuildCategory = 4,
    GuildAnnouncement = 5,
    AnnouncementThread = 10,
    PublicThread = 11,
    PrivateThread = 12,
    GuildStageVoice = 13,
    GuildForum = 15,
};

struct Channel {
    Snowflake id;
    ChannelType type = ChannelType::GuildText;
    std::optional<Snowflake> guildId;
    std::optional<Snowflake> parentId;
    std::string name;
    int position = 0;
    bool nsfw = false;
    int flags = 0;

    static Channel from(const discord::Event &event);
};

struct TextChannel : Channel {
    std::optional<std::string> topic;
    std::optional<Snowflake> lastMessageId;
    int rateLimitPerUser = 0;

    static TextChannel from(const discord::Event &event);
};

struct VoiceChannel : Channel {
    int bitrate = 0;
    int userLimit = 0;
    std::optional<std::string> rtcRegion;

    static VoiceChannel from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, Channel &c) {
    j.at("id").get_to(c.id);
    c.type = static_cast<ChannelType>(j.value("type", 0));
    c.position = j.value("position", 0);
    c.name = j.value("name", std::string{});
    c.nsfw = j.value("nsfw", false);
    c.flags = j.value("flags", 0);

    if (const auto it = j.find("guild_id"); it != j.end() && !it->is_null())
        c.guildId = it->get<Snowflake>();
    if (const auto it = j.find("parent_id"); it != j.end() && !it->is_null())
        c.parentId = it->get<Snowflake>();
}

inline void from_json(const nlohmann::json &j, TextChannel &c) {
    from_json(j, static_cast<Channel &>(c));
    c.rateLimitPerUser = j.value("rate_limit_per_user", 0);

    if (const auto it = j.find("topic"); it != j.end() && !it->is_null())
        c.topic = it->get<std::string>();
    if (const auto it = j.find("last_message_id"); it != j.end() && !it->is_null())
        c.lastMessageId = it->get<Snowflake>();
}

inline void from_json(const nlohmann::json &j, VoiceChannel &c) {
    from_json(j, static_cast<Channel &>(c));
    c.bitrate = j.value("bitrate", 0);
    c.userLimit = j.value("user_limit", 0);

    if (const auto it = j.find("rtc_region"); it != j.end() && !it->is_null())
        c.rtcRegion = it->get<std::string>();
}

}
