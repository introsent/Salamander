#pragma once

#include <vector>
#include <functional>

class DeletionQueue {
public:
    // Push a deletion function onto the queue
    void push(std::function<void()> func) {
        m_deletors.push_back(std::move(func));
    }

    // Execute all deletors in reverse order
    void flush() {
        for (auto it = m_deletors.rbegin(); it != m_deletors.rend(); ++it) {
            (*it)();
        }
        m_deletors.clear();
    }

private:
    std::vector<std::function<void()>> m_deletors;
};
