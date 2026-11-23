#include <echopair/devices/endpoint_change_watcher.h>

#include "../platform/com_support.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace echopair::devices {
namespace {

using Microsoft::WRL::ComPtr;
using echopair::platform::check_hresult;
using echopair::platform::ComApartment;

struct HandleCloser {
    void operator()(void* handle) const noexcept
    {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }
};

using EventHandle = std::unique_ptr<void, HandleCloser>;

class EndpointNotificationClient final : public IMMNotificationClient {
public:
    explicit EndpointNotificationClient(HANDLE event) noexcept : event_(event) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interface_id, void** object) override
    {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;

        if (IsEqualIID(interface_id, __uuidof(IUnknown)) ||
            IsEqualIID(interface_id, __uuidof(IMMNotificationClient))) {
            *object = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return references_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG remaining = references_.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (remaining == 0) {
            delete this;
        }
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override
    {
        return signal_change();
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override
    {
        return signal_change();
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override
    {
        return signal_change();
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override
    {
        return signal_change();
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        return signal_change();
    }

private:
    HRESULT signal_change() noexcept
    {
        return SetEvent(event_) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    std::atomic<ULONG> references_{1};
    HANDLE event_;
};

}

class EndpointChangeWatcher::Impl final {
public:
    Impl()
        : event_(CreateEventW(nullptr, FALSE, FALSE, nullptr))
    {
        if (!event_) {
            throw std::runtime_error("CreateEventW failed for endpoint notifications");
        }

        check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      IID_PPV_ARGS(enumerator_.GetAddressOf())),
                      "CoCreateInstance(MMDeviceEnumerator)");

        auto* client = new EndpointNotificationClient(static_cast<HANDLE>(event_.get()));
        callback_.Attach(client);
        check_hresult(enumerator_->RegisterEndpointNotificationCallback(callback_.Get()),
                      "IMMDeviceEnumerator::RegisterEndpointNotificationCallback");
        registered_ = true;
    }

    ~Impl()
    {
        if (registered_) {
            enumerator_->UnregisterEndpointNotificationCallback(callback_.Get());
        }
    }

    bool wait_for_change(std::chrono::milliseconds timeout)
    {
        DWORD wait_milliseconds = 0;
        if (timeout == std::chrono::milliseconds::max()) {
            wait_milliseconds = INFINITE;
        } else if (timeout > std::chrono::milliseconds::zero()) {
            const auto max_wait = static_cast<std::int64_t>(INFINITE - 1);
            wait_milliseconds = static_cast<DWORD>(
                std::min(timeout.count(), max_wait));
        }

        const DWORD result =
            WaitForSingleObject(static_cast<HANDLE>(event_.get()), wait_milliseconds);
        if (result == WAIT_OBJECT_0) {
            return true;
        }
        if (result == WAIT_TIMEOUT) {
            return false;
        }

        const DWORD error = GetLastError();
        throw std::runtime_error("WaitForSingleObject failed for endpoint notifications "
                                 "(Win32 error " + std::to_string(error) + ')');
    }

private:
    ComApartment apartment_;
    EventHandle event_;
    ComPtr<IMMDeviceEnumerator> enumerator_;
    ComPtr<IMMNotificationClient> callback_;
    bool registered_ = false;
};

EndpointChangeWatcher::EndpointChangeWatcher()
    : impl_(std::make_unique<Impl>())
{
}

EndpointChangeWatcher::~EndpointChangeWatcher() = default;

bool EndpointChangeWatcher::wait_for_change(std::chrono::milliseconds timeout)
{
    return impl_->wait_for_change(timeout);
}

}
