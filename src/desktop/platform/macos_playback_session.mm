#if defined(__APPLE__)
#include "beatglass/desktop/macos_playback_session.hpp"

#include <algorithm>
#include <memory>

namespace beatglass::macos {

MacAudioUnitPlayback::~MacAudioUnitPlayback() { stop(); }

bool MacAudioUnitPlayback::start(std::uint32_t rate, std::uint16_t channels,
                                 std::size_t max_samples,
                                 std::uint64_t epoch) noexcept {
  stop();
  if (!rate || !channels || !max_samples || !epoch || channels > 2) return false;
  if (!session_.start(epoch)) return false;
  owned_queue_ = std::make_unique<PlaybackPcmQueue>(2, max_samples);
  scratch_ = std::make_unique<float[]>(max_samples);
  max_samples_ = max_samples;
  channels_ = channels;
  AudioComponentDescription desc{};
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  const auto component = AudioComponentFindNext(nullptr, &desc);
  if (!component || AudioComponentInstanceNew(component, &unit_) != noErr) {
    stop();
    return false;
  }
  AudioStreamBasicDescription format{};
  format.mSampleRate = rate;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
  format.mFramesPerPacket = 1;
  format.mChannelsPerFrame = channels;
  format.mBitsPerChannel = 32;
  format.mBytesPerFrame = sizeof(float) * channels;
  format.mBytesPerPacket = format.mBytesPerFrame;
  if (AudioUnitSetProperty(unit_, kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Input, 0, &format,
                           sizeof(format)) != noErr) {
    stop();
    return false;
  }
  AURenderCallbackStruct callback{&MacAudioUnitPlayback::render, this};
  if (AudioUnitSetProperty(unit_, kAudioUnitProperty_SetRenderCallback,
                           kAudioUnitScope_Input, 0, &callback,
                           sizeof(callback)) != noErr ||
      AudioUnitInitialize(unit_) != noErr || AudioOutputUnitStart(unit_) != noErr) {
    stop();
    return false;
  }
  queue_ = owned_queue_.get();
  return true;
}

void MacAudioUnitPlayback::stop() noexcept {
  if (unit_) {
    AudioOutputUnitStop(unit_);
    AudioUnitUninitialize(unit_);
    AudioComponentInstanceDispose(unit_);
    unit_ = nullptr;
  }
  queue_ = nullptr;
  owned_queue_.reset();
  scratch_.reset();
  max_samples_ = channels_ = 0;
  session_.stop();
}

bool MacAudioUnitPlayback::submit(const AudioBlock& block) noexcept {
  return queue_ && session_.state() == PlaybackState::playing && block.epoch == session_.epoch() && queue_->push(block);
}

bool MacAudioUnitPlayback::pause() noexcept {
  if (!unit_ || !session_.pause()) return false;
  if (AudioOutputUnitStop(unit_) != noErr) {
    session_.resume();
    return false;
  }
  return true;
}

bool MacAudioUnitPlayback::resume() noexcept {
  if (!unit_ || !session_.resume()) return false;
  if (AudioOutputUnitStart(unit_) != noErr) {
    session_.pause();
    return false;
  }
  return true;
}

OSStatus MacAudioUnitPlayback::render(void* ref, AudioUnitRenderActionFlags*,
                                      const AudioTimeStamp*, UInt32,
                                      UInt32 frames, AudioBufferList* io) noexcept {
  auto* self = static_cast<MacAudioUnitPlayback*>(ref);
  if (!self || !self->queue_ || !self->scratch_ || !io || !io->mNumberBuffers) return noErr;
  AudioBlock block{};
  const auto count = std::min<std::size_t>(frames * self->channels_, self->max_samples_);
  if (!self->queue_->pop(std::span<float>(self->scratch_.get(), count), block)) {
    for (UInt32 i = 0; i < io->mNumberBuffers; ++i)
      std::fill_n(static_cast<float*>(io->mBuffers[i].mData), io->mBuffers[i].mDataByteSize / sizeof(float), 0.0f);
    return noErr;
  }
  const auto samples = std::min<std::size_t>(count, block.samples.size());
  auto* out = static_cast<float*>(io->mBuffers[0].mData);
  std::copy_n(block.samples.data(), samples, out);
  std::fill(out + samples, out + count, 0.0f);
  return noErr;
}

}  // namespace beatglass::macos
#endif
