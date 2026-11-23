#pragma once

#include <windows.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace echopair::platform {

inline void check_hresult(HRESULT result, const char* operation)
{
    if (FAILED(result)) {
        std::ostringstream message;
        message << operation << " failed (HRESULT 0x"
                << std::uppercase << std::hex
                << static_cast<std::uint32_t>(result) << ')';
        throw std::runtime_error(message.str());
    }
}

class ComApartment final {
public:
    ComApartment()
    {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(result) && result != RPC_E_CHANGED_MODE) {
            check_hresult(result, "CoInitializeEx");
        }
        initialized_ = SUCCEEDED(result);
    }

    ~ComApartment()
    {
        if (initialized_) {
            CoUninitialize();
        }
    }

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

private:
    bool initialized_ = false;
};

}
