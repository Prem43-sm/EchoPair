#pragma once

#include <chrono>
#include <string>

namespace echopair::audio {

void play_test_tone_on_endpoint(
    const std::wstring& endpoint_id,
    std::chrono::milliseconds duration);

}
