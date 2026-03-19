#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "gateway/Gateway.hpp"

namespace {

void require(bool condition, const std::string &message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testSubscribeAndUnsubscribe() {
    discord::Gateway gateway;

    std::mutex mutex;
    std::condition_variable cv;
    int callbackCount = 0;

    const auto subscriptionId = gateway.subscribe("unit_test_event", [&](const discord::Event &) {
        {
            std::lock_guard lock(mutex);
            ++callbackCount;
        }
        cv.notify_one();
    });

    gateway.publish(discord::Event{"unit_test_event", "payload"});

    {
        std::unique_lock lock(mutex);
        const bool delivered =
            cv.wait_for(lock, std::chrono::seconds(2), [&]() { return callbackCount == 1; });
        require(delivered, "callback was not invoked for subscribed event");
    }

    require(gateway.unsubscribe(subscriptionId),
            "unsubscribe should return true for active subscription");

    gateway.publish(discord::Event{"unit_test_event", "payload"});
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
        std::lock_guard lock(mutex);
        require(callbackCount == 1, "callback should not fire after unsubscribe");
    }
}

void testConcurrentPublish() {
    discord::Gateway gateway;

    std::mutex mutex;
    std::condition_variable cv;
    int callbackCount = 0;

    gateway.subscribeAll([&](const discord::Event &) {
        {
            std::lock_guard lock(mutex);
            ++callbackCount;
        }
        cv.notify_one();
    });

    constexpr int kPublisherCount = 4;
    constexpr int kEventsPerPublisher = 250;
    constexpr int kExpectedEvents = kPublisherCount * kEventsPerPublisher;

    std::vector<std::thread> publishers;
    publishers.reserve(kPublisherCount);

    for (int i = 0; i < kPublisherCount; ++i) {
        publishers.emplace_back([&gateway]() {
            for (int eventIndex = 0; eventIndex < kEventsPerPublisher; ++eventIndex) {
                gateway.publish(discord::Event{"concurrent_event", "payload"});
            }
        });
    }

    for (auto &publisher : publishers) {
        publisher.join();
    }

    {
        std::unique_lock lock(mutex);
        const bool allEventsProcessed = cv.wait_for(
            lock, std::chrono::seconds(5), [&]() { return callbackCount == kExpectedEvents; });
        require(allEventsProcessed,
                "not all published events were delivered to subscribers in concurrent scenario");
    }
}

}

int main() {
    try {
        testSubscribeAndUnsubscribe();
        testConcurrentPublish();
        std::cout << "All tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
