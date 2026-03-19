#pragma once

#include <chrono>
#include <string>

namespace discord {

struct Event {
    std::string mName;
    std::string mPayload;
    std::chrono::system_clock::time_point mReceivedAt{std::chrono::system_clock::now()};
};

}
