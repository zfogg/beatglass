#include "beatglass/capture/coreaudio_tap_internal.hpp"
#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/AudioHardwareTapping.h>
#import <CoreAudio/CATapDescription.h>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <span>
#include <thread>
#include <utility>
#include <vector>

namespace beatglass::coreaudio {
TapFailure classify_osstatus(std::int32_t status) noexcept {
  if (status == kAudioDevicePermissionsError) return TapFailure::permission_denied;
  if (status == kAudioDeviceUnsupportedFormatError) return TapFailure::unsupported;
  if (status == kAudioHardwareBadDeviceError || status == kAudioHardwareBadObjectError ||
      status == kAudioHardwareBadStreamError) return TapFailure::device_lost;
  return TapFailure::backend_error;
}
namespace {
AudioObjectPropertyAddress property(AudioObjectPropertySelector selector, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal) {
  return {selector, scope, kAudioObjectPropertyElementMain};
}
template<class T> OSStatus get(AudioObjectID object, AudioObjectPropertySelector selector, T& value) {
  auto address = property(selector); UInt32 size = sizeof(value);
  const auto result = AudioObjectGetPropertyData(object, &address, 0, nullptr, &size, &value);
  return result == noErr && size != sizeof(value) ? kAudioHardwareBadPropertySizeError : result;
}
bool compatible(const AudioStreamBasicDescription& f) {
  return f.mFormatID == kAudioFormatLinearPCM && (f.mFormatFlags & kAudioFormatFlagIsFloat) &&
    !(f.mFormatFlags & kAudioFormatFlagIsNonInterleaved) && f.mBitsPerChannel == 32 &&
    f.mChannelsPerFrame > 0 && f.mChannelsPerFrame <= 32 && f.mFramesPerPacket == 1 &&
    f.mBytesPerFrame == f.mChannelsPerFrame * sizeof(float) &&
    std::isfinite(f.mSampleRate) && f.mSampleRate >= 1 && f.mSampleRate <= std::numeric_limits<std::uint32_t>::max() &&
    std::floor(f.mSampleRate) == f.mSampleRate;
}
bool first_output_stream(AudioObjectID device, AudioStreamID& stream) {
  auto address = property(kAudioDevicePropertyStreams, kAudioDevicePropertyScopeOutput);
  UInt32 size = 0;
  if (AudioObjectGetPropertyDataSize(device, &address, 0, nullptr, &size) != noErr ||
      size < sizeof(AudioStreamID) || size % sizeof(AudioStreamID)) return false;
  std::vector<AudioStreamID> streams(size / sizeof(AudioStreamID));
  if (AudioObjectGetPropertyData(device, &address, 0, nullptr, &size, streams.data()) != noErr ||
      size < sizeof(AudioStreamID) || size % sizeof(AudioStreamID) || size > streams.size() * sizeof(AudioStreamID)) return false;
  stream = streams[0]; return true;
}
}

class OutputTap::Impl {
public:
  AudioObjectID tap{kAudioObjectUnknown}, aggregate{kAudioObjectUnknown}, device{kAudioObjectUnknown};
  AudioStreamID stream{kAudioObjectUnknown};
  AudioDeviceIOProcID io{};
  AudioStreamBasicDescription format{};
  std::unique_ptr<CaptureFifo> fifo;
  CallbackGate callback_gate;
  TapStatus current{};
  std::atomic<bool> callback_fault{false};
  bool started{};
  NativeTimeline timeline;

  static OSStatus callback(AudioObjectID, const AudioTimeStamp*, const AudioBufferList* input,
                           const AudioTimeStamp* time, AudioBufferList*, const AudioTimeStamp*, void* context) noexcept {
    auto& self = *static_cast<Impl*>(context);
    const auto entry = self.callback_gate.try_enter();
    if (entry != CallbackEntry::entered) {
      if (entry == CallbackEntry::busy) self.callback_fault.store(true, std::memory_order_release);
      return noErr;
    }
    struct Leave { CallbackGate& gate; ~Leave() { gate.leave(); } } leave{self.callback_gate};
    if (!input || input->mNumberBuffers != 1) { self.callback_fault.store(true, std::memory_order_release); return noErr; }
    const auto& buffer = input->mBuffers[0];
    if (!buffer.mData || buffer.mNumberChannels != self.format.mChannelsPerFrame ||
        buffer.mDataByteSize % self.format.mBytesPerFrame != 0) {
      self.callback_fault.store(true, std::memory_order_release); return noErr;
    }
    const auto frames = buffer.mDataByteSize / self.format.mBytesPerFrame;
    const bool sample_valid = time && (time->mFlags & kAudioTimeStampSampleTimeValid) && std::isfinite(time->mSampleTime);
    const bool gap = self.timeline.observe(sample_valid ? time->mSampleTime : 0, sample_valid, frames);
    const bool host_valid = time && (time->mFlags & kAudioTimeStampHostTimeValid);
    self.fifo->push(std::span<const float>(static_cast<const float*>(buffer.mData), buffer.mDataByteSize / sizeof(float)),
                    host_valid ? time->mHostTime : 0, host_valid, gap,
                    sample_valid ? time->mSampleTime : 0, sample_valid);
    if (self.fifo->exhausted()) self.callback_fault.store(true, std::memory_order_release);
    return noErr;
  }
  bool cleanup() noexcept {
    callback_gate.close();
    while (!callback_gate.drained()) std::this_thread::yield();
    const CleanupOps operations{
      [](void* p) noexcept { auto& s = *static_cast<Impl*>(p); if (!s.started) return true;
        if (AudioDeviceStop(s.aggregate, s.io) != noErr) return false; s.started = false; return true; },
      [](void* p) noexcept { auto& s = *static_cast<Impl*>(p); if (!s.io) return true;
        if (AudioDeviceDestroyIOProcID(s.aggregate, s.io) != noErr) return false; s.io = nullptr; return true; },
      [](void* p) noexcept { auto& s = *static_cast<Impl*>(p); if (s.aggregate == kAudioObjectUnknown) return true;
        if (AudioHardwareDestroyAggregateDevice(s.aggregate) != noErr) return false; s.aggregate = kAudioObjectUnknown; return true; },
      [](void* p) noexcept { auto& s = *static_cast<Impl*>(p); if (s.tap == kAudioObjectUnknown) return true;
        if (AudioHardwareDestroyProcessTap(s.tap) != noErr) return false; s.tap = kAudioObjectUnknown; return true; }
    };
    if (!cleanup_in_order(this, operations)) return false;
    device = kAudioObjectUnknown; fifo.reset(); return true;
  }
  bool fail(const char* operation, OSStatus code) {
    const bool cleaned = cleanup();
    const auto failure = cleaned ? classify_osstatus(code) : TapFailure::backend_error;
    std::string message = std::string(operation) + " (OSStatus " + std::to_string(code) + ")";
    if (failure == TapFailure::permission_denied) message += "; grant macOS audio recording permission and retry";
    if (!cleaned) message += "; cleanup failed";
    current = {TapState::error, std::move(message), failure}; return false;
  }
  bool fail_message(const char* message, TapFailure failure) {
    const bool cleaned = cleanup();
    current = {TapState::error, cleaned ? message : "Core Audio cleanup failed", cleaned ? failure : TapFailure::backend_error};
    return false;
  }
};

OutputTap::OutputTap() : impl_(std::make_unique<Impl>()) {}
OutputTap::~OutputTap() {
  stop();
  // If Core Audio refuses to quiesce or destroy the IOProc, its context must
  // remain alive. Deliberately retain resources rather than risk callback UAF.
  if (impl_ && (impl_->io || impl_->aggregate != kAudioObjectUnknown || impl_->tap != kAudioObjectUnknown)) (void)impl_.release();
}

bool OutputTap::start(const std::string& uid, std::uint64_t epoch) {
  if (!stop()) return false;
  if (uid.empty() || uid.find('\0') != std::string::npos || uid.find_first_not_of(" \t\r\n") == std::string::npos) {
    impl_->current = {TapState::error, "Output UID required", TapFailure::invalid_argument}; return false;
  }
  if (epoch == 0) {
    impl_->current = {TapState::error, "Nonzero capture epoch required", TapFailure::invalid_argument}; return false;
  }
  if (@available(macOS 14.2, *)) {
    @autoreleasepool {
      NSString* output = [[NSString alloc] initWithBytes:uid.data() length:uid.size() encoding:NSUTF8StringEncoding];
      if (!output) { impl_->current = {TapState::error, "Output UID is not valid UTF-8", TapFailure::invalid_argument}; return false; }
      auto address = property(kAudioHardwarePropertyTranslateUIDToDevice);
      CFStringRef key = (__bridge CFStringRef)output;
      UInt32 size = sizeof(impl_->device);
      OSStatus s = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, sizeof(key), &key, &size, &impl_->device);
      if (s != noErr) return impl_->fail("Resolve output device", s);
      if (impl_->device == kAudioObjectUnknown) return impl_->fail_message("Output device not found", TapFailure::not_found);
      if (!first_output_stream(impl_->device, impl_->stream)) return impl_->fail("Output has no readable stream", kAudioHardwareBadDeviceError);
      CATapDescription* description = [[CATapDescription alloc] initExcludingProcesses:@[] andDeviceUID:output withStream:0];
      description.name = @"Beatglass Output Tap"; description.privateTap = YES; description.muteBehavior = CATapUnmuted;
      s = AudioHardwareCreateProcessTap(description, &impl_->tap);
      if (s != noErr) return impl_->fail("Create process tap", s);
      s = get(impl_->tap, kAudioTapPropertyFormat, impl_->format);
      if (s != noErr) return impl_->fail("Read tap format", s);
      if (!compatible(impl_->format)) return impl_->fail_message("Tap format is not interleaved float32 PCM", TapFailure::unsupported);
      CFStringRef tapUID = nullptr;
      s = get(impl_->tap, kAudioTapPropertyUID, tapUID);
      if (s != noErr || !tapUID) return impl_->fail("Read tap UID", s);
      NSString* aggregateUID = [[NSUUID UUID] UUIDString];
      NSDictionary* configuration = @{
        @kAudioAggregateDeviceNameKey: @"Beatglass Tap Input",
        @kAudioAggregateDeviceUIDKey: aggregateUID,
        @kAudioAggregateDeviceIsPrivateKey: @YES,
        @kAudioAggregateDeviceTapAutoStartKey: @YES,
        @kAudioAggregateDeviceTapListKey: @[@{@kAudioSubTapUIDKey: (__bridge NSString*)tapUID}]
      };
      s = AudioHardwareCreateAggregateDevice((__bridge CFDictionaryRef)configuration, &impl_->aggregate);
      CFRelease(tapUID);
      if (s != noErr) return impl_->fail("Create tap aggregate device", s);
      auto inputAddress = property(kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeInput);
      AudioStreamBasicDescription inputFormat{};
      size = sizeof(inputFormat);
      s = AudioObjectGetPropertyData(impl_->aggregate, &inputAddress, 0, nullptr, &size, &inputFormat);
      if (s != noErr || size != sizeof(inputFormat)) return impl_->fail("Read aggregate input format", s);
      if (!compatible(inputFormat) || inputFormat.mSampleRate != impl_->format.mSampleRate ||
          inputFormat.mChannelsPerFrame != impl_->format.mChannelsPerFrame ||
          inputFormat.mFormatFlags != impl_->format.mFormatFlags || inputFormat.mBytesPerFrame != impl_->format.mBytesPerFrame) {
        return impl_->fail_message("Aggregate input format differs from the tap format", TapFailure::unsupported);
      }
      impl_->fifo = std::make_unique<CaptureFifo>(epoch, static_cast<std::uint32_t>(impl_->format.mSampleRate), impl_->format.mChannelsPerFrame);
      impl_->callback_fault.store(false, std::memory_order_relaxed); impl_->timeline = NativeTimeline{};
      impl_->callback_gate.reopen();
      s = AudioDeviceCreateIOProcID(impl_->aggregate, &Impl::callback, impl_.get(), &impl_->io);
      if (s != noErr) return impl_->fail("Create tap IOProc", s);
      s = AudioDeviceStart(impl_->aggregate, impl_->io);
      if (s != noErr) return impl_->fail("Start tap IOProc (check system audio permission)", s);
      impl_->started = true; impl_->current = {TapState::running, {}}; return true;
    }
  }
  impl_->current = {TapState::error, "Core Audio taps require macOS 14.2 or newer", TapFailure::unsupported}; return false;
}

TapStatus OutputTap::status() {
  if (impl_->current.state != TapState::running) return impl_->current;
  UInt32 alive = 0;
  AudioStreamID stream = kAudioObjectUnknown;
  AudioStreamBasicDescription format{};
  const auto alive_status = get(impl_->device, kAudioDevicePropertyDeviceIsAlive, alive);
  const bool device_lost = alive_status != noErr || !alive;
  const bool stream_lost = !first_output_stream(impl_->device, stream) || stream != impl_->stream;
  if (impl_->callback_fault.load(std::memory_order_acquire) || device_lost ||
      stream_lost ||
      get(impl_->tap, kAudioTapPropertyFormat, format) != noErr ||
      format.mSampleRate != impl_->format.mSampleRate || format.mChannelsPerFrame != impl_->format.mChannelsPerFrame ||
      format.mFormatID != impl_->format.mFormatID || format.mFormatFlags != impl_->format.mFormatFlags ||
      format.mBytesPerFrame != impl_->format.mBytesPerFrame || format.mBitsPerChannel != impl_->format.mBitsPerChannel ||
      format.mFramesPerPacket != impl_->format.mFramesPerPacket ||
      format.mBytesPerPacket != impl_->format.mBytesPerPacket) {
    const char* message = (device_lost || stream_lost)
        ? "Output device or stream disappeared; select it again and restart with a fresh epoch"
        : "Output format changed or tap callback failed; restart with a fresh epoch";
    impl_->current = {TapState::interrupted, message,
                      (device_lost || stream_lost) ? TapFailure::device_lost : TapFailure::backend_error};
  }
  return impl_->current;
}
std::optional<AudioFormat> OutputTap::format() const noexcept {
  if (impl_->current.state != TapState::running) return std::nullopt;
  const auto channels = static_cast<std::uint16_t>(impl_->format.mChannelsPerFrame);
  const auto layout = channels == 1 ? ChannelLayout::mono : channels == 2 ? ChannelLayout::stereo : ChannelLayout::multichannel;
  return AudioFormat{static_cast<std::uint32_t>(impl_->format.mSampleRate), channels, layout};
}
bool OutputTap::pop(CaptureBlock& block) {
  if (status().state != TapState::running || !impl_->fifo->pop(block)) return false;
  return status().state == TapState::running;
}
bool OutputTap::stop() noexcept {
  if (impl_->cleanup()) { impl_->current.state = TapState::stopped; impl_->current.message.clear(); impl_->current.failure.reset(); return true; }
  impl_->current.state = TapState::error;
  impl_->current.message = "Core Audio cleanup failed; retry stop before restart";
  impl_->current.failure = TapFailure::backend_error;
  return false;
}
std::uint64_t OutputTap::dropped_frames() const noexcept { return impl_->fifo ? impl_->fifo->dropped_frames() : 0; }
}
