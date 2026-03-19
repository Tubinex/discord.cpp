#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "Client.hpp"

namespace {
std::atomic<bool> gRunning{true};
void onSignal(int) { gRunning = false; }
}

static const char *env(const char *key, const char *fallback = nullptr) {
    const char *val = std::getenv(key);
    return (val && val[0]) ? val : fallback;
}

int main() {
    const char *token = env("DISCORD_TOKEN");
    if (!token) {
        std::cerr << "DISCORD_TOKEN is not set\n";
        return 1;
    }
    std::string tokenStr(token);
    while (!tokenStr.empty() && static_cast<unsigned char>(tokenStr.back()) <= ' ') {
        tokenStr.pop_back();
    }

    const discord::ClientType clientType = std::string(env("DISCORD_CLIENT_TYPE", "bot")) == "user"
                                               ? discord::ClientType::User
                                               : discord::ClientType::Bot;

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    discord::Client client(tokenStr, clientType);

    if (const char *ua = env("DISCORD_USER_AGENT")) {
        client.gateway().setUserAgent(ua);
        client.http().setUserAgent(ua);
    }

    client.onReady([](const discord::Ready &r) {
        std::cout << "Logged in as " << r.user.username << " (session: " << r.sessionId << ")\n";
    });

    client.onMessageCreate([](const discord::Message &msg) {
        std::cout << "[MSG] " << msg.author.username << ": " << msg.content << '\n';
    });

    client.onMessageDelete([](const discord::MessageDelete &msg) {
        std::cout << "[DEL] message " << msg.id << " in channel " << msg.channelId << '\n';
    });

    client.onChannelCreate(
        [](const discord::Channel &ch) { std::cout << "[CHANNEL+] #" << ch.name << '\n'; });

    client.onGuildMemberUpdate([](const discord::Member &member) {
        if (member.user)
            std::cout << "[MEMBER] " << member.user->username << " updated\n";
    });

    client.gateway().subscribe("socket_error", [](const discord::Event &e) {
        std::cerr << "Connection error: " << e.mPayload << '\n';
    });

    std::cout << "Connecting... (Ctrl+C to stop)\n";
    client.connect();

    while (gRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down...\n";
    client.disconnect();

    return 0;
}
