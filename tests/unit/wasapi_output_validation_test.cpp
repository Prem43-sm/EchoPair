#include <echopair/audio/wasapi_output.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool rejects(const std::vector<std::wstring>& endpoint_ids,
             std::chrono::milliseconds duration)
{
    try {
        echopair::audio::play_test_tone_on_endpoints(endpoint_ids, duration);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}

int main()
{
    using namespace std::chrono_literals;

    if (!rejects({}, 1s) ||
        !rejects({L"one", L"two", L"three"}, 1s) ||
        !rejects({L"same", L"same"}, 1s) ||
        !rejects({L""}, 1s) ||
        !rejects({L"endpoint"}, 0ms)) {
        std::cerr << "Audio output validation accepted invalid input.\n";
        return 1;
    }
    return 0;
}
