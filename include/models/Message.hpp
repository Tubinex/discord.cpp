#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "models/Member.hpp"
#include "models/Snowflake.hpp"
#include "models/User.hpp"

namespace discord {

struct Event;

struct Attachment {
    Snowflake id;
    std::string filename;
    std::string url;
    std::string proxyUrl;
    int size = 0;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<std::string> contentType;
};

inline void from_json(const nlohmann::json &j, Attachment &a) {
    j.at("id").get_to(a.id);
    j.at("filename").get_to(a.filename);
    j.at("url").get_to(a.url);
    a.proxyUrl = j.value("proxy_url", std::string{});
    a.size = j.value("size", 0);

    if (const auto it = j.find("width"); it != j.end() && !it->is_null())
        a.width = it->get<int>();
    if (const auto it = j.find("height"); it != j.end() && !it->is_null())
        a.height = it->get<int>();
    if (const auto it = j.find("content_type"); it != j.end() && !it->is_null())
        a.contentType = it->get<std::string>();
}

struct Message {
    Snowflake id;
    Snowflake channelId;
    std::optional<Snowflake> guildId;
    User author;
    std::optional<Member> member;
    std::string content;
    std::string timestamp;
    std::optional<std::string> editedTimestamp;
    bool tts = false;
    bool mentionEveryone = false;
    bool pinned = false;
    int type = 0;
    int flags = 0;
    std::vector<User> mentions;
    std::vector<Snowflake> mentionRoles;
    std::vector<Attachment> attachments;

    static Message from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, Message &m) {
    j.at("id").get_to(m.id);
    j.at("channel_id").get_to(m.channelId);
    j.at("author").get_to(m.author);
    m.content = j.value("content", std::string{});
    m.timestamp = j.value("timestamp", std::string{});
    m.tts = j.value("tts", false);
    m.mentionEveryone = j.value("mention_everyone", false);
    m.pinned = j.value("pinned", false);
    m.type = j.value("type", 0);
    m.flags = j.value("flags", 0);

    if (const auto it = j.find("guild_id"); it != j.end() && !it->is_null())
        m.guildId = it->get<Snowflake>();
    if (const auto it = j.find("member"); it != j.end() && !it->is_null())
        m.member = it->get<Member>();
    if (const auto it = j.find("edited_timestamp"); it != j.end() && !it->is_null())
        m.editedTimestamp = it->get<std::string>();
    if (const auto it = j.find("mentions"); it != j.end() && it->is_array())
        m.mentions = it->get<std::vector<User>>();
    if (const auto it = j.find("mention_roles"); it != j.end() && it->is_array())
        m.mentionRoles = it->get<std::vector<Snowflake>>();
    if (const auto it = j.find("attachments"); it != j.end() && it->is_array())
        m.attachments = it->get<std::vector<Attachment>>();
}

struct MessageDelete {
    Snowflake id;
    Snowflake channelId;
    std::optional<Snowflake> guildId;

    static MessageDelete from(const discord::Event &event);
};

inline void from_json(const nlohmann::json &j, MessageDelete &m) {
    j.at("id").get_to(m.id);
    j.at("channel_id").get_to(m.channelId);
    if (const auto it = j.find("guild_id"); it != j.end() && !it->is_null())
        m.guildId = it->get<Snowflake>();
}

}
