// DeletionQueue.h
#pragma once

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>

class DeletionQueue
{
public:
    using Deletor = std::function<void()>;

    static DeletionQueue& get()
    {
        static DeletionQueue instance;
        return instance;
    }


    // Add or replace deletor by name, keeping the original insertion order
    void pushFunction(const std::string& name, Deletor&& deletor)
    {
        for (auto& entry : entries)
        {
            if (entry.name == name)
            {
                entry.deletor = std::move(deletor); // Replace in-place
                return;
            }
        }

        // Not found, add new
        entries.push_back({ name, std::move(deletor) });
    }

    // Flush and run all deletors in reverse order
    void flush()
    {
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
        {
            if (it->deletor)
                it->deletor();
        }
        entries.clear();
    }

private:
    struct Entry
    {
        std::string name;
        Deletor deletor;
    };

    std::vector<Entry> entries;
};

