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
#include <utility>
#include <vector>

namespace beatglass::coreaudio {
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
  TapStatus current{};
  std::atomic<bool> callback_fault{false};
  bool started{};
  bool have_sample_time{};
  Float64 expected_sample_time{};

  static OSStatus callback(AudioObjectID, const AudioTimeStamp*, const AudioBufferList* input,
                           const AudioTimeStamp* time, AudioBufferList*, const AudioTimeStamp*, void* context) noexcept {
    auto& self = *static_cast<Impl*>(context);
    if (!input || input->mNumberBuffers != 1) { self.callback_fault.store(true, std::memory_order_release); return noErr; }
    const auto& buffer = input->mBuffers[0];
    if (!buffer.mData || buffer.mNumberChannels != self.format.mChannelsPerFrame ||
        buffer.mDataByteSize % self.format.mBytesPerFrame != 0) {
      self.callback_fault.store(true, std::memory_order_release); return noErr;
    }
    const auto frames = buffer.mDataByteSize / self.format.mBytesPerFrame;
    const bool sample_valid = time && (time->mFlags & kAudioTimeStampSampleTimeValid);
    const bool gap = sample_valid && self.have_sample_time && time->mSampleTime != self.expected_sample_time;
    if (sample_valid) { self.expected_sample_time = time->mSampleTime + frames; self.have_sample_time = true; }
    else { self.have_sample_time = false; }
    const bool host_valid = time && (time->mFlags & kAudioTimeStampHostTimeValid);
    self.fifo->push(std::span<const float>(static_cast<const float*>(buffer.mData), buffer.mDataByteSize / sizeof(float)),
                    host_valid ? time->mHostTime : 0, host_valid, gap);
    if (self.fifo->exhausted()) self.callback_fault.store(true, std::memory_order_release);
    return noErr;
  }
  bool cleanup() noexcept {
    if (started) {
      if (AudioDeviceStop(aggregate, io) != noErr) return false;
      started = false;
    }
    if (io) {
      if (AudioDeviceDestroyIOProcID(aggregate, io) != noErr) return false;
      io = nullptr;
    }
    if (aggregate != kAudioObjectUnknown) { AudioHardwareDestroyAggregateDevice(aggregate); aggregate = kAudioObjectUnknown; }
    if (tap != kAudioObjectUnknown) { AudioHardwareDestroyProcessTap(tap); tap = kAudioObjectUnknown; }
    device = kAudioObjectUnknown; fifo.reset(); return true;
  }
  bool fail(const char* operation, OSStatus code) {
    const bool cleaned = cleanup();
    current = {TapState::error, std::string(operation) + " (OSStatus " + std::to_string(code) + ")" + (cleaned ? "" : "; cleanup failed")}; return false;
  }
};

OutputTap::OutputTap() : impl_(std::make_unique<Impl>()) {}
OutputTap::~OutputTap() {
  stop();
  // If Core Audio refuses to quiesce or destroy the IOProc, its context must
  // remain alive. Deliberately retain resources rather than risk callback UAF.
  if (impl_ && impl_->io) (void)impl_.release();
}

bool OutputTap::start(const std::string& uid, std::uint64_t epoch) {
  stop();
  if (impl_->io) return false;
  if (uid.empty() || epoch == 0) { impl_->current = {TapState::error, "Output UID and nonzero epoch required"}; return false; }
  if (@available(macOS 14.2, *)) {
    @autoreleasepool {
      NSString* output = [[NSString alloc] initWithBytes:uid.data() length:uid.size() encoding:NSUTF8StringEncoding];
      if (!output) { impl_->current = {TapState::error, "Output UID is not valid UTF-8"}; return false; }
      auto address = property(kAudioHardwarePropertyTranslateUIDToDevice);
      CFStringRef key = (__bridge CFStringRef)output;
      UInt32 size = sizeof(impl_->device);
      OSStatus s = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, sizeof(key), &key, &size, &impl_->device);
      if (s != noErr || impl_->device == kAudioObjectUnknown) return impl_->fail("Output device not found", s);
      if (!first_output_stream(impl_->device, impl_->stream)) return impl_->fail("Output has no readable stream", kAudioHardwareBadDeviceError);
      CATapDescription* description = [[CATapDescription alloc] initExcludingProcesses:@[] andDeviceUID:output withStream:0];
      description.name = @"Beatglass Output Tap"; description.privateTap = YES; description.muteBehavior = CATapUnmuted;
      s = AudioHardwareCreateProcessTap(description, &impl_->tap);
      if (s != noErr) return impl_->fail("Create process tap", s);
      s = get(impl_->tap, kAudioTapPropertyFormat, impl_->format);
      if (s != noErr) return impl_->fail("Read tap format", s);
      if (!compatible(impl_->format)) { impl_->cleanup(); impl_->current = {TapState::error, "Tap format is not interleaved float32 PCM"}; return false; }
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
        impl_->cleanup(); impl_->current = {TapState::error, "Aggregate input format differs from the tap format"}; return false;
      }
      impl_->fifo = std::make_unique<CaptureFifo>(epoch, static_cast<std::uint32_t>(impl_->format.mSampleRate), impl_->format.mChannelsPerFrame);
      impl_->callback_fault.store(false, std::memory_order_relaxed); impl_->have_sample_time = false;
      s = AudioDeviceCreateIOProcID(impl_->aggregate, &Impl::callback, impl_.get(), &impl_->io);
      if (s != noErr) return impl_->fail("Create tap IOProc", s);
      s = AudioDeviceStart(impl_->aggregate, impl_->io);
      if (s != noErr) return impl_->fail("Start tap IOProc (check system audio permission)", s);
      impl_->started = true; impl_->current = {TapState::running, {}}; return true;
    }
  }
  impl_->current = {TapState::error, "Core Audio taps require macOS 14.2 or newer"}; return false;
}

TapStatus OutputTap::status() {
  if (impl_->current.state != TapState::running) return impl_->current;
  UInt32 alive = 0;
  AudioStreamID stream = kAudioObjectUnknown;
  AudioStreamBasicDescription format{};
  if (impl_->callback_fault.load(std::memory_order_acquire) || get(impl_->device, kAudioDevicePropertyDeviceIsAlive, alive) != noErr || !alive ||
      !first_output_stream(impl_->device, stream) || stream != impl_->stream ||
      get(impl_->tap, kAudioTapPropertyFormat, format) != noErr ||
      format.mSampleRate != impl_->format.mSampleRate || format.mChannelsPerFrame != impl_->format.mChannelsPerFrame ||
      format.mFormatID != impl_->format.mFormatID || format.mFormatFlags != impl_->format.mFormatFlags ||
      format.mBytesPerFrame != impl_->format.mBytesPerFrame || format.mBitsPerChannel != impl_->format.mBitsPerChannel) {
    impl_->current = {TapState::interrupted, "Output disappeared, format changed, or tap callback failed; restart with a fresh epoch"};
  }
  return impl_->current;
}
bool OutputTap::pop(CaptureBlock& block) {
  if (status().state != TapState::running || !impl_->fifo->pop(block)) return false;
  return status().state == TapState::running;
}
void OutputTap::stop() noexcept {
  if (impl_->cleanup()) { impl_->current.state = TapState::stopped; impl_->current.message.clear(); }
  else { impl_->current.state = TapState::error; }
}
std::uint64_t OutputTap::dropped_frames() const noexcept { return impl_->fifo ? impl_->fifo->dropped_frames() : 0; }
}
