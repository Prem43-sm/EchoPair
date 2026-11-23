#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

#include <echopair/audio/wasapi_output.h>
#include <echopair/devices/audio_endpoint.h>

int wmain(int argc, wchar_t* argv[])
{
    std::wcout << L"EchoPair " << ECHO_PAIR_VERSION
               << L" - One Sound. Multiple Ears.\n";

    try {
        const auto endpoints = echopair::devices::enumerate_playback_endpoints();
        if (endpoints.empty()) {
            std::wcout << L"No playback endpoints were returned by Windows Core Audio.\n";
            return argc == 1 ? 0 : 1;
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

        if (argc == 1) {
            return 0;
        }

        if (argc == 2 && std::wstring(argv[1]) == L"--test-tone") {
            const auto endpoint = std::find_if(
                endpoints.begin(), endpoints.end(),
                [](const auto& candidate) {
                    return candidate.is_default_for_console &&
                           candidate.state == echopair::devices::EndpointState::active;
                });
            if (endpoint == endpoints.end()) {
                std::cerr << "No active default console playback endpoint is available.\n";
                return 1;
            }

            std::wcout << L"Playing a 440 Hz test tone for 3 seconds on the default endpoint.\n";
            echopair::audio::play_test_tone_on_endpoint(
                endpoint->id, std::chrono::seconds(3));
            return 0;
        }

        std::wcerr << L"Usage: EchoPair.exe [--test-tone]\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "EchoPair operation failed: " << error.what() << '\n';
        return 1;
    }
}
