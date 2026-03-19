#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "gateway/Gateway.hpp"
#include "http/Client.hpp"

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

    discord::Gateway gateway(tokenStr, clientType);
    discord::HttpClient http(tokenStr, clientType);

    if (const char *ua = env("DISCORD_USER_AGENT")) {
        gateway.setUserAgent(ua);
        http.setUserAgent(ua);
    }

    if (const char *v = env("DISCORD_OS_VERSION")) {
        gateway.setOsVersion(v);
    }
    if (const char *v = env("DISCORD_OS_ARCH")) {
        gateway.setOsArch(v);
    }
    if (const char *v = env("DISCORD_DISTRO")) {
        gateway.setDistro(v);
    }
    if (const char *v = env("DISCORD_DISPLAY_SERVER")) {
        gateway.setDisplayServer(v);
    }
    if (const char *v = env("DISCORD_WINDOW_MANAGER")) {
        gateway.setWindowManager(v);
    }
    if (const char *v = env("DISCORD_BUILD_NUMBER")) {
        try {
            gateway.setClientBuildNumber(std::stoi(v));
        } catch (...) {
        }
    }

    gateway.enableMessageDump("messages.json");
    gateway.subscribe("socket_open", [](const discord::Event &) {
        std::cout << "Connected to Discord gateway\n";
    });

    gateway.subscribe("socket_close", [](const discord::Event &event) {
        std::cout << "Connection closed: " << event.mPayload << '\n';
    });

    gateway.subscribe("socket_error", [](const discord::Event &event) {
        std::cerr << "Connection error: " << event.mPayload << '\n';
    });

    gateway.subscribeAll([](const discord::Event &event) {
        std::cout << "[" << event.mName << "] " << event.mPayload << '\n';
    });

    std::cout << "Connecting... (Ctrl+C to stop)\n";
    gateway.connect();

    while (gRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down...\n";
    gateway.disconnect();

    return 0;
}
