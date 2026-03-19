#include "events/Dispatcher.hpp"

#include <stdexcept>

namespace {
constexpr const char *kWildcardEventName = "*";
}

namespace discord::detail {

Dispatcher::Dispatcher() = default;

Dispatcher::~Dispatcher() { stop(); }

void Dispatcher::start() {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return;
    }

    mWorkerThread = std::thread([this]() { run(); });
}

void Dispatcher::stop() {
    bool expected = true;
    if (!mRunning.compare_exchange_strong(expected, false)) {
        return;
    }

    mQueueCv.notify_all();

    if (mWorkerThread.joinable()) {
        mWorkerThread.join();
    }
}

bool Dispatcher::isRunning() const noexcept { return mRunning.load(); }

SubscriptionId Dispatcher::subscribe(std::string eventName, EventCallback callback) {
    if (!callback) {
        throw std::invalid_argument("callback must be valid");
    }

    if (eventName.empty()) {
        eventName = kWildcardEventName;
    }

    const SubscriptionId subscriptionId = mNextSubscriptionId.fetch_add(1);

    std::unique_lock lock(mSubscribersMutex);
    mSubscribers.emplace(subscriptionId, Subscriber{std::move(eventName), std::move(callback)});
    return subscriptionId;
}

bool Dispatcher::unsubscribe(SubscriptionId subscriptionId) {
    std::unique_lock lock(mSubscribersMutex);
    return mSubscribers.erase(subscriptionId) > 0;
}

void Dispatcher::publish(Event event) {
    {
        std::lock_guard lock(mQueueMutex);
        mEventQueue.push(std::move(event));
    }
    mQueueCv.notify_one();
}

void Dispatcher::run() {
    while (true) {
        Event event;

        {
            std::unique_lock lock(mQueueMutex);
            mQueueCv.wait(lock, [this]() { return !mRunning.load() || !mEventQueue.empty(); });

            if (!mRunning.load() && mEventQueue.empty()) {
                break;
            }

            event = std::move(mEventQueue.front());
            mEventQueue.pop();
        }

        dispatch(event);
    }
}

void Dispatcher::dispatch(const Event &event) {
    std::vector<EventCallback> callbacks;

    {
        std::shared_lock lock(mSubscribersMutex);
        callbacks.reserve(mSubscribers.size());

        for (const auto &[subscriptionId, subscriber] : mSubscribers) {
            (void)subscriptionId;
            if (subscriber.mEventName == kWildcardEventName ||
                subscriber.mEventName == event.mName) {
                callbacks.push_back(subscriber.mCallback);
            }
        }
    }

    for (const auto &callback : callbacks) {
        try {
            callback(event);
        } catch (...) {
        }
    }
}

}
