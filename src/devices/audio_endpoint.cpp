#include <echopair/devices/audio_endpoint.h>

#include "../platform/com_support.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <wrl/client.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace echopair::devices {
namespace {

using Microsoft::WRL::ComPtr;
using echopair::platform::check_hresult;
using echopair::platform::ComApartment;

class PropVariant final {
public:
    PropVariant()
    {
        PropVariantInit(&value_);
    }

    ~PropVariant()
    {
        PropVariantClear(&value_);
    }

    PROPVARIANT* get() noexcept
    {
        return &value_;
    }

    const PROPVARIANT& value() const noexcept
    {
        return value_;
    }

    PropVariant(const PropVariant&) = delete;
    PropVariant& operator=(const PropVariant&) = delete;

private:
    PROPVARIANT value_{};
};

EndpointState to_endpoint_state(DWORD state) noexcept
{
    switch (state) {
    case DEVICE_STATE_ACTIVE:
        return EndpointState::active;
    case DEVICE_STATE_DISABLED:
        return EndpointState::disabled;
    case DEVICE_STATE_NOTPRESENT:
        return EndpointState::not_present;
    case DEVICE_STATE_UNPLUGGED:
        return EndpointState::unplugged;
    default:
        return EndpointState::unknown;
    }
}

std::wstring get_endpoint_id(IMMDevice* device)
{
    LPWSTR raw_id = nullptr;
    check_hresult(device->GetId(&raw_id), "IMMDevice::GetId");
    const std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> id(raw_id, &CoTaskMemFree);
    if (!id) {
        throw std::runtime_error("IMMDevice::GetId returned an empty endpoint identifier");
    }
    return id.get();
}

std::wstring get_endpoint_name(IMMDevice* device)
{
    ComPtr<IPropertyStore> properties;
    check_hresult(device->OpenPropertyStore(STGM_READ, properties.GetAddressOf()),
                  "IMMDevice::OpenPropertyStore");

    PropVariant friendly_name;
    check_hresult(properties->GetValue(PKEY_Device_FriendlyName, friendly_name.get()),
                  "IPropertyStore::GetValue(PKEY_Device_FriendlyName)");
    if (friendly_name.value().vt != VT_LPWSTR || friendly_name.value().pwszVal == nullptr) {
        throw std::runtime_error("Windows returned no string friendly name for an audio endpoint");
    }

    return friendly_name.value().pwszVal;
}

}

const wchar_t* endpoint_state_name(EndpointState state) noexcept
{
    switch (state) {
    case EndpointState::active:
        return L"Active";
    case EndpointState::disabled:
        return L"Disabled";
    case EndpointState::not_present:
        return L"Not present";
    case EndpointState::unplugged:
        return L"Unplugged";
    case EndpointState::unknown:
        return L"Unknown";
    }
    return L"Unknown";
}

std::vector<AudioEndpoint> enumerate_playback_endpoints()
{
    ComApartment apartment;
    ComPtr<IMMDeviceEnumerator> enumerator;
    check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                   IID_PPV_ARGS(enumerator.GetAddressOf())),
                  "CoCreateInstance(MMDeviceEnumerator)");

    std::wstring default_endpoint_id;
    ComPtr<IMMDevice> default_endpoint;
    const HRESULT default_result =
        enumerator->GetDefaultAudioEndpoint(eRender, eConsole, default_endpoint.GetAddressOf());
    if (SUCCEEDED(default_result)) {
        default_endpoint_id = get_endpoint_id(default_endpoint.Get());
    } else if (default_result != E_NOTFOUND) {
        check_hresult(default_result, "IMMDeviceEnumerator::GetDefaultAudioEndpoint");
    }

    ComPtr<IMMDeviceCollection> collection;
    check_hresult(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL,
                                                  collection.GetAddressOf()),
                  "IMMDeviceEnumerator::EnumAudioEndpoints");

    UINT count = 0;
    check_hresult(collection->GetCount(&count), "IMMDeviceCollection::GetCount");

    std::vector<AudioEndpoint> endpoints;
    endpoints.reserve(count);
    for (UINT index = 0; index < count; ++index) {
        ComPtr<IMMDevice> device;
        check_hresult(collection->Item(index, device.GetAddressOf()),
                      "IMMDeviceCollection::Item");

        const std::wstring id = get_endpoint_id(device.Get());
        DWORD native_state = 0;
        check_hresult(device->GetState(&native_state), "IMMDevice::GetState");

        endpoints.push_back({
            id,
            get_endpoint_name(device.Get()),
            to_endpoint_state(native_state),
            !default_endpoint_id.empty() && id == default_endpoint_id
        });
    }

    return endpoints;
}

}
