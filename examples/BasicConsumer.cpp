#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "gateway/Gateway.hpp"

int main(int argc, char **argv) {
    const std::string websocketUrl = argc > 1 ? argv[1] : "ws://127.0.0.1:8765";

    discord::Gateway gateway;

    gateway.subscribe("socket_open", [&gateway](const discord::Event &) {
        std::cout << "WebSocket opened, sending sample payload..." << std::endl;
        gateway.send("{\"event\":\"demo\",\"message\":\"hello from discord\"}");
    });

    gateway.subscribe("demo", [](const discord::Event &event) {
        std::cout << "Received demo event: " << event.mPayload << std::endl;
    });

    gateway.subscribe("raw_message", [](const discord::Event &event) {
        std::cout << "Received raw message: " << event.mPayload << std::endl;
    });

    gateway.subscribe("socket_error", [](const discord::Event &event) {
        std::cerr << "WebSocket error: " << event.mPayload << std::endl;
    });

    std::cout << "Connecting to: " << websocketUrl << '\n';
    gateway.connect(websocketUrl);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    gateway.disconnect();

    return 0;
}
