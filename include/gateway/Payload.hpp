#pragma once

#include <cstdint>
#include <string>

namespace discord {

enum class Opcode : std::uint8_t {
    Dispatch = 0,
    Heartbeat = 1,
    Identify = 2,
    Hello = 10,
    HeartbeatAck = 11,
};

struct Payload {
    Opcode op = Opcode::Dispatch;
    std::string d;
    std::string t;
    int s = -1;
};

}
