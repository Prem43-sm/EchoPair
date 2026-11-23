#pragma once

#include <string>
#include <vector>

namespace echopair::devices {

enum class EndpointState {
    active,
    disabled,
    not_present,
    unplugged,
    unknown
};

struct AudioEndpoint {
    std::wstring id;
    std::wstring name;
    EndpointState state;
    bool is_default_for_console;
};

std::vector<AudioEndpoint> enumerate_playback_endpoints();
const wchar_t* endpoint_state_name(EndpointState state) noexcept;

}
