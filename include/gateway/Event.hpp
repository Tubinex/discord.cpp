#pragma once

namespace discord::events {

// Connection
inline constexpr const char *READY = "READY";

// Messages
inline constexpr const char *MESSAGE_CREATE = "MESSAGE_CREATE";
inline constexpr const char *MESSAGE_UPDATE = "MESSAGE_UPDATE";
inline constexpr const char *MESSAGE_DELETE = "MESSAGE_DELETE";
inline constexpr const char *MESSAGE_ACK = "MESSAGE_ACK";

// Channels
inline constexpr const char *CHANNEL_CREATE = "CHANNEL_CREATE";
inline constexpr const char *CHANNEL_UPDATE = "CHANNEL_UPDATE";
inline constexpr const char *CHANNEL_DELETE = "CHANNEL_DELETE";
inline constexpr const char *CHANNEL_UNREAD_UPDATE = "CHANNEL_UNREAD_UPDATE";

// Threads
inline constexpr const char *THREAD_CREATE = "THREAD_CREATE";
inline constexpr const char *THREAD_UPDATE = "THREAD_UPDATE";

// Guilds
inline constexpr const char *GUILD_UPDATE = "GUILD_UPDATE";
inline constexpr const char *GUILD_MEMBER_UPDATE = "GUILD_MEMBER_UPDATE";

// Users
inline constexpr const char *USER_UPDATE = "USER_UPDATE";
inline constexpr const char *USER_CONNECTIONS_UPDATE = "USER_CONNECTIONS_UPDATE";
inline constexpr const char *USER_SETTINGS_UPDATE = "USER_SETTINGS_UPDATE";
inline constexpr const char *USER_SETTINGS_PROTO_UPDATE = "USER_SETTINGS_PROTO_UPDATE";

// Voice
inline constexpr const char *VOICE_CHANNEL_STATUS_UPDATE = "VOICE_CHANNEL_STATUS_UPDATE";
inline constexpr const char *VOICE_CHANNEL_START_TIME_UPDATE = "VOICE_CHANNEL_START_TIME_UPDATE";

// Misc
inline constexpr const char *BURST_CREDIT_BALANCE_UPDATE = "BURST_CREDIT_BALANCE_UPDATE";
inline constexpr const char *CONVERSATION_SUMMARY_UPDATE = "CONVERSATION_SUMMARY_UPDATE";
inline constexpr const char *GENERIC_PUSH_NOTIFICATION_SENT = "GENERIC_PUSH_NOTIFICATION_SENT";
inline constexpr const char *QUESTS_USER_STATUS_UPDATE = "QUESTS_USER_STATUS_UPDATE";
inline constexpr const char *REACTION_NOTIFICATION_SENT = "REACTION_NOTIFICATION_SENT";

}
