#include <echopair/devices/audio_endpoint.h>

#include <iostream>
#include <set>
#include <string>

int main()
{
    try {
        const auto endpoints = echopair::devices::enumerate_playback_endpoints();
        std::set<std::wstring> identifiers;

        for (const auto& endpoint : endpoints) {
            if (endpoint.id.empty()) {
                std::cerr << "Windows returned an endpoint with an empty identifier.\n";
                return 1;
            }
            if (!identifiers.insert(endpoint.id).second) {
                std::cerr << "Windows returned duplicate endpoint identifiers.\n";
                return 1;
            }
            switch (endpoint.state) {
            case echopair::devices::EndpointState::active:
            case echopair::devices::EndpointState::disabled:
            case echopair::devices::EndpointState::not_present:
            case echopair::devices::EndpointState::unplugged:
            case echopair::devices::EndpointState::unknown:
                break;
            default:
                std::cerr << "Endpoint has an invalid state value.\n";
                return 1;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Endpoint enumeration integration test failed: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}
