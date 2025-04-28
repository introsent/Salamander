// DeletionQueue.h
#pragma once

#include <functional>
#include <vector>

// A global, process-lifetime queue of Vulkan cleanup calls.
// Use DeletionQueue::get().pushFunction(...) anywhere.
class DeletionQueue {
public:
    // Get the one and only instance.
    static DeletionQueue& get() {
        static DeletionQueue instance;
        return instance;
    }

    // Enqueue a destroyer lambda.
    void pushFunction(std::function<void()>&& fn) {
        deletors.push_back(std::move(fn));
    }

    // Flush in reverse order. Call at shutdown.
    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
            (*it)();
        }
        deletors.clear();
    }

    // Whether the queue is empty.
    bool empty() const { return deletors.empty(); }

private:
    DeletionQueue() = default;
    ~DeletionQueue() = default;

    // non-copyable, non-movable
    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;
    DeletionQueue(DeletionQueue&&) = delete;
    DeletionQueue& operator=(DeletionQueue&&) = delete;

    std::unordered_map<std::string, std::function<void()>> m_deletors;
};
