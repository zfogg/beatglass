#include "beatglass/capture/source.hpp"

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

#include <string>
#include <utility>
#include <vector>

namespace beatglass::coreaudio {
namespace {
std::string utf8(CFStringRef value) {
  if (!value) return {};
  const auto capacity = CFStringGetMaximumSizeForEncoding(CFStringGetLength(value), kCFStringEncodingUTF8) + 1;
  std::vector<char> bytes(static_cast<std::size_t>(capacity));
  if (!CFStringGetCString(value, bytes.data(), capacity, kCFStringEncodingUTF8)) return {};
  return bytes.data();
}

bool property_string(AudioObjectID id, AudioObjectPropertySelector selector, std::string& result) {
  AudioObjectPropertyAddress address{selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
  CFStringRef value = nullptr;
  UInt32 size = sizeof(value);
  if (AudioObjectGetPropertyData(id, &address, 0, nullptr, &size, &value) != noErr || !value) return false;
  result = utf8(value);
  CFRelease(value);
  return !result.empty();
}
}

// Control-thread-only discovery; an error is distinct from an empty list.
std::vector<SourceInfo> enumerate_outputs(std::string& error) {
  error.clear();
  AudioObjectPropertyAddress address{kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
  UInt32 size = 0;
  OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size);
  if (status != noErr || size % sizeof(AudioDeviceID)) {
    error = "Cannot query Core Audio devices (OSStatus " + std::to_string(status) + ")";
    return {};
  }
  std::vector<AudioDeviceID> devices(size / sizeof(AudioDeviceID));
  if (size != 0) {
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, devices.data());
    if (status != noErr || size % sizeof(AudioDeviceID)) {
      error = "Cannot read Core Audio devices (OSStatus " + std::to_string(status) + ")";
      return {};
    }
    devices.resize(size / sizeof(AudioDeviceID));
  }
  std::vector<SourceInfo> result;
  for (const auto device : devices) {
    AudioObjectPropertyAddress streams{kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMain};
    UInt32 stream_bytes = 0;
    // A disappearing or unreadable device cannot safely be offered as a source.
    if (AudioObjectGetPropertyDataSize(device, &streams, 0, nullptr, &stream_bytes) != noErr || stream_bytes == 0) continue;
    std::string uid, name;
    if (!property_string(device, kAudioDevicePropertyDeviceUID, uid) || !property_string(device, kAudioObjectPropertyName, name)) continue;
    result.push_back({std::move(uid), std::move(name), true, false});
  }
  return result;
}
}
