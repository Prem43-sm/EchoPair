#include <iostream>

#include <echopair/devices/audio_endpoint.h>

int main()
{
    std::wcout << L"EchoPair " << ECHO_PAIR_VERSION
               << L" - One Sound. Multiple Ears.\n";

    try {
        const auto endpoints = echopair::devices::enumerate_playback_endpoints();
        if (endpoints.empty()) {
            std::wcout << L"No playback endpoints were returned by Windows Core Audio.\n";
            return 0;
        }

        std::wcout << L"Windows playback endpoints:\n";
        for (const auto& endpoint : endpoints) {
            std::wcout << L"  " << endpoint.name << L" ["
                       << echopair::devices::endpoint_state_name(endpoint.state)
                       << L"]";
            if (endpoint.is_default_for_console) {
                std::wcout << L" (default)";
            }
            std::wcout << L"\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Endpoint enumeration failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
