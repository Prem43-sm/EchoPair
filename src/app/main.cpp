#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <echopair/audio/wasapi_output.h>
#include <echopair/devices/audio_endpoint.h>
#include <echopair/devices/endpoint_change_watcher.h>

namespace {

const echopair::devices::AudioEndpoint& endpoint_at(
    const std::vector<echopair::devices::AudioEndpoint>& endpoints,
    const wchar_t* argument)
{
    const std::wstring value(argument);
    std::size_t parsed_characters = 0;
    const auto index = std::stoul(value, &parsed_characters);
    if (parsed_characters != value.size() || index == 0 || index > endpoints.size()) {
        throw std::invalid_argument("Endpoint index is outside the displayed list");
    }

    const auto& endpoint = endpoints[index - 1];
    if (endpoint.state != echopair::devices::EndpointState::active) {
        throw std::invalid_argument("The selected endpoint is not active");
    }
    return endpoint;
}

void print_endpoints(const std::vector<echopair::devices::AudioEndpoint>& endpoints)
{
    if (endpoints.empty()) {
        std::wcout << L"No playback endpoints were returned by Windows Core Audio.\n";
        return;
    }

    std::wcout << L"Windows playback endpoints:\n";
    for (std::size_t index = 0; index < endpoints.size(); ++index) {
        const auto& endpoint = endpoints[index];
        std::wcout << L"  " << index + 1 << L". " << endpoint.name << L" ["
                   << echopair::devices::endpoint_state_name(endpoint.state)
                   << L"]";
        if (endpoint.is_default_for_console) {
            std::wcout << L" (default)";
        }
        std::wcout << L"\n";
    }
}

}

int wmain(int argc, wchar_t* argv[])
{
    std::wcout << L"EchoPair " << ECHO_PAIR_VERSION
               << L" - One Sound. Multiple Ears.\n";

    try {
        if (argc == 2 && std::wstring(argv[1]) == L"--watch") {
            echopair::devices::EndpointChangeWatcher watcher;
            while (true) {
                print_endpoints(echopair::devices::enumerate_playback_endpoints());
                std::wcout << L"Waiting for endpoint changes. Press Ctrl+C to exit.\n";
                watcher.wait_for_change(std::chrono::milliseconds::max());
                std::wcout << L"Endpoint change detected.\n";
            }
        }

        const auto endpoints = echopair::devices::enumerate_playback_endpoints();
        if (endpoints.empty()) {
            std::wcout << L"No playback endpoints were returned by Windows Core Audio.\n";
            return argc == 1 ? 0 : 1;
        }

        print_endpoints(endpoints);

        if (argc == 1) {
            return 0;
        }

        if (std::wstring(argv[1]) == L"--test-tone" && argc <= 4) {
            std::vector<std::wstring> selected_endpoint_ids;
            if (argc == 2) {
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
                selected_endpoint_ids.push_back(endpoint->id);
            } else {
                for (int argument = 2; argument < argc; ++argument) {
                    selected_endpoint_ids.push_back(endpoint_at(endpoints, argv[argument]).id);
                }
            }

            std::wcout << L"Playing the same 440 Hz test tone for 3 seconds on "
                       << selected_endpoint_ids.size() << L" endpoint(s).\n";
            echopair::audio::play_test_tone_on_endpoints(
                selected_endpoint_ids, std::chrono::seconds(3));
            return 0;
        }

        std::wcerr << L"Usage: EchoPair.exe [--watch | --test-tone [endpoint-index [endpoint-index]]]\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "EchoPair operation failed: " << error.what() << '\n';
        return 1;
    }
}
