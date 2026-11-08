#include "windows_endpoints.hpp"
#include <windows.h>
#include <mmdeviceapi.h>
#include <initguid.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <stdexcept>
#include <utility>

namespace beatglass::windows {
namespace {
template<class T> struct ComPtr {
  T* value{};
  ~ComPtr() { if (value) value->Release(); }
  T** out() { return &value; }
  T* operator->() const { return value; }
};
struct ComApartment {
  HRESULT result{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
  ~ComApartment() { if (SUCCEEDED(result)) CoUninitialize(); }
};
struct CoString {
  wchar_t* value{};
  ~CoString() { CoTaskMemFree(value); }
};
struct PropertyValue {
  PROPVARIANT value;
  PropertyValue() { PropVariantInit(&value); }
  ~PropertyValue() { PropVariantClear(&value); }
};
void check(HRESULT hr, const char* what) {
  if (FAILED(hr)) throw std::runtime_error(std::string(what) + " (HRESULT " + std::to_string(static_cast<unsigned long>(hr)) + ")");
}
std::string utf8(const wchar_t* value) {
  if (!value) throw std::runtime_error("Missing endpoint string");
  const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, nullptr, 0, nullptr, nullptr);
  if (!size) throw std::runtime_error("Invalid endpoint string");
  std::string result(static_cast<std::size_t>(size), '\0');
  if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, result.data(), size, nullptr, nullptr))
    throw std::runtime_error("Endpoint string conversion failed");
  result.pop_back();
  return result;
}
}
EndpointResult enumerate_render_endpoints() {
  try {
    ComApartment apartment;
    // An already initialized STA is also a valid COM apartment for this call.
    if (apartment.result != RPC_E_CHANGED_MODE) check(apartment.result, "COM initialization failed");
    ComPtr<IMMDeviceEnumerator> enumerator;
    check(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(enumerator.out())), "Create device enumerator failed");
    ComPtr<IMMDevice> default_device;
    const HRESULT default_result = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, default_device.out());
    std::string default_id;
    if (default_result != E_NOTFOUND) {
      check(default_result, "Get default render endpoint failed");
      CoString id;
      check(default_device->GetId(&id.value), "Get default endpoint ID failed");
      default_id = utf8(id.value);
    }
    ComPtr<IMMDeviceCollection> devices;
    check(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, devices.out()), "Enumerate render endpoints failed");
    UINT count{};
    check(devices->GetCount(&count), "Get endpoint count failed");
    EndpointResult result;
    for (UINT i = 0; i < count; ++i) {
      ComPtr<IMMDevice> device;
      check(devices->Item(i, device.out()), "Get endpoint failed");
      CoString id;
      check(device->GetId(&id.value), "Get endpoint ID failed");
      ComPtr<IPropertyStore> properties;
      check(device->OpenPropertyStore(STGM_READ, properties.out()), "Open endpoint properties failed");
      PropertyValue name;
      check(properties->GetValue(PKEY_Device_FriendlyName, &name.value), "Get endpoint name failed");
      if (name.value.vt != VT_LPWSTR) throw std::runtime_error("Endpoint name has unexpected type");
      auto endpoint_id = utf8(id.value);
      result.endpoints.push_back({endpoint_id, utf8(name.value.pwszVal), endpoint_id == default_id});
    }
    return result;
  } catch (const std::exception& error) {
    return {{}, error.what()};
  }
}
bool contains_render_endpoint(const EndpointResult& snapshot, const std::string& id) noexcept { if (id.empty() || !snapshot.error.empty()) return false; for (const auto& endpoint : snapshot.endpoints) if (endpoint.id == id) return true; return false; }
}
