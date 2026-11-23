#pragma once

#include <chrono>
#include <memory>

namespace echopair::devices {

class EndpointChangeWatcher final {
public:
    EndpointChangeWatcher();
    ~EndpointChangeWatcher();

    EndpointChangeWatcher(const EndpointChangeWatcher&) = delete;
    EndpointChangeWatcher& operator=(const EndpointChangeWatcher&) = delete;

    bool wait_for_change(std::chrono::milliseconds timeout);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
