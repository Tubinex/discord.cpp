#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "models/Event.hpp"
#include "models/Subscription.hpp"

namespace discord::detail {

class Dispatcher {
  public:
    using EventCallback = std::function<void(const Event &)>;

    Dispatcher();
    ~Dispatcher();

    Dispatcher(const Dispatcher &) = delete;
    Dispatcher &operator=(const Dispatcher &) = delete;
    Dispatcher(Dispatcher &&) = delete;
    Dispatcher &operator=(Dispatcher &&) = delete;

    void start();
    void stop();
    bool isRunning() const noexcept;

    SubscriptionId subscribe(std::string eventName, EventCallback callback);
    bool unsubscribe(SubscriptionId subscriptionId);

    void publish(Event event);

  private:
    struct Subscriber {
        std::string mEventName;
        EventCallback mCallback;
    };

    void run();
    void dispatch(const Event &event);

    std::unordered_map<SubscriptionId, Subscriber> mSubscribers;
    mutable std::shared_mutex mSubscribersMutex;

    std::queue<Event> mEventQueue;
    mutable std::mutex mQueueMutex;
    std::condition_variable mQueueCv;

    std::thread mWorkerThread;
    std::atomic<bool> mRunning{false};
    std::atomic<SubscriptionId> mNextSubscriptionId{1};
};

}
