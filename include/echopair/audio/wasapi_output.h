#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace echopair::audio {

void play_test_tone_on_endpoint(
    const std::wstring& endpoint_id,
    std::chrono::milliseconds duration);
void play_test_tone_on_endpoints(
    const std::vector<std::wstring>& endpoint_ids,
    std::chrono::milliseconds duration);

}
