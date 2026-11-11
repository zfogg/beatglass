#include "windows_loopback.hpp"
#include "windows_device_position.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <thread>

namespace beatglass::windows {
namespace {
template<class T> struct ComPtr {
  T* p{};
  ~ComPtr() { if (p) p->Release(); }
  T** out() { return &p; }
  T* operator->() const { return p; }
};
struct Format { WAVEFORMATEX* p{}; ~Format() { CoTaskMemFree(p); } };
bool float_format(const WAVEFORMATEX* f) {
  if (f->wBitsPerSample != 32 || f->nChannels == 0 || f->nSamplesPerSec == 0 || f->nBlockAlign != f->nChannels * sizeof(float)) return false;
  if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
  if (f->wFormatTag != WAVE_FORMAT_EXTENSIBLE || f->cbSize < 22) return false;
  const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f);
  constexpr GUID ieee_float{3, 0, 0x10, {0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71}};
  return IsEqualGUID(ext->SubFormat, ieee_float) && ext->Samples.wValidBitsPerSample == 32;
}
std::wstring widen(const std::string& text) {
  if (text.empty() || text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) return {};
  const auto n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
  if (!n) return {};
  std::wstring result(static_cast<std::size_t>(n), L'\0');
  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), n)) return {};
  return result;
}
}
struct Loopback::Impl {
  static_assert(std::atomic<unsigned>::is_always_lock_free && std::atomic<std::uint64_t>::is_always_lock_free);
  // Keep enough bounded headroom for a control-loop tick while preserving the
  // allocation-free capture path.  WASAPI periods can be substantially shorter
  // than the CLI's 10 ms refresh interval on shared-mode endpoints.
  static constexpr unsigned slots = 64;
  std::array<LoopbackPacket, slots> queue{};
  std::atomic<unsigned> read{0}, write{0};
  std::atomic<std::uint64_t> dropped{0};
  std::shared_ptr<LoopbackLossToken> loss = std::make_shared<LoopbackLossToken>();
  LoopbackLossEvents loss_events{*loss};
  std::atomic<State> current{State::stopped};
  std::atomic<std::int32_t> error{0};
  std::atomic<bool> cancel{false};
  std::array<Event, 4> events{};
  std::atomic<unsigned> event_read{0}, event_write{0};
  std::uint64_t event_sequence{}, epoch{};
  std::thread worker;
  bool gap{};

  void publish(State state, HRESULT hr) noexcept {
    const auto index = event_write.load(std::memory_order_relaxed);
    const auto next = (index + 1) % events.size();
    // A start emits at most running followed by interrupted, so the four-slot
    // queue cannot overflow before stop joins and resets it.
    events[index] = {state, ++event_sequence, epoch, static_cast<std::int32_t>(hr)};
    event_write.store(static_cast<unsigned>(next), std::memory_order_release);
    error.store(static_cast<std::int32_t>(hr), std::memory_order_relaxed);
    current.store(state, std::memory_order_release);
  }
  void fail(HRESULT hr) { loss_events.failure(); publish(State::interrupted, hr); }
  void push(const float* data, unsigned frames, unsigned channels, unsigned rate, std::uint64_t epoch,
            std::uint64_t position, std::uint64_t qpc, bool valid, bool discontinuity, bool silent) noexcept {
    const unsigned next = (write.load(std::memory_order_relaxed) + 1) % slots;
    if (next == read.load(std::memory_order_acquire)) { dropped.fetch_add(frames, std::memory_order_relaxed); loss_events.queueFull(); gap = true; return; }
    auto& out = queue[write.load(std::memory_order_relaxed)];
    out.sample_count = frames * channels; out.channels = channels; out.sample_rate = rate;
    out.epoch = epoch; out.loss_stamp = loss_events.packetStamp(discontinuity); out.first_frame = position; out.qpc_100ns = qpc;
    out.timestamp_valid = valid; out.discontinuity = gap || discontinuity;
    if (silent) std::fill_n(out.samples.data(), out.sample_count, 0.0f);
    else std::memcpy(out.samples.data(), data, out.sample_count * sizeof(float));
    gap = false;
    write.store(next, std::memory_order_release);
  }
  void run(std::wstring id, std::uint64_t epoch, std::shared_ptr<LoopbackStartAuthorization> authorization) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) { fail(hr); return; }
    {
      ComPtr<IMMDeviceEnumerator> enumerator;
      ComPtr<IMMDevice> device;
      ComPtr<IMMEndpoint> endpoint;
      ComPtr<IAudioClient> audio;
      ComPtr<IAudioCaptureClient> capture;
      Format format;
      hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(enumerator.out()));
      if (SUCCEEDED(hr)) hr = enumerator->GetDevice(id.c_str(), device.out());
      if (SUCCEEDED(hr)) hr = device->QueryInterface(__uuidof(IMMEndpoint), reinterpret_cast<void**>(endpoint.out()));
      EDataFlow flow{};
      if (SUCCEEDED(hr)) hr = endpoint->GetDataFlow(&flow);
      if (SUCCEEDED(hr) && flow != eRender) hr = E_INVALIDARG;
      if (SUCCEEDED(hr)) hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(audio.out()));
      if (SUCCEEDED(hr)) hr = audio->GetMixFormat(&format.p);
      if (SUCCEEDED(hr) && (!float_format(format.p) || format.p->nChannels > LoopbackPacket::max_samples)) hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
      if (SUCCEEDED(hr)) hr = audio->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 1000000, 0, format.p, nullptr);
      if (SUCCEEDED(hr)) hr = audio->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(capture.out()));
      if (SUCCEEDED(hr) && authorization && authorization->cancelled()) hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
      if (SUCCEEDED(hr)) hr = audio->Start();
      if (SUCCEEDED(hr) && authorization && authorization->cancelled()) { audio->Stop(); hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); }
      if (FAILED(hr)) fail(hr);
      else {
        publish(State::running, S_OK);
        std::uint64_t position = 0;
        DevicePositionContinuity device_position;
        unsigned polls = 0;
        while (!cancel.load(std::memory_order_acquire) && !(authorization && authorization->cancelled())) {
          Sleep(10);
          if (++polls == 100) {
            polls = 0;
            DWORD device_state{};
            hr = device->GetState(&device_state);
            if (SUCCEEDED(hr) && device_state != DEVICE_STATE_ACTIVE) hr = AUDCLNT_E_DEVICE_INVALIDATED;
            if (FAILED(hr)) { fail(hr); break; }
          }
          UINT32 available{};
          hr = capture->GetNextPacketSize(&available);
          while (SUCCEEDED(hr) && available && !cancel.load(std::memory_order_acquire) && !(authorization && authorization->cancelled())) {
            BYTE* data{}; UINT32 frames{}; DWORD flags{}; UINT64 device_pos{}, qpc{};
            hr = capture->GetBuffer(&data, &frames, &flags, &device_pos, &qpc);
            if (FAILED(hr)) break;
            const auto channels = format.p->nChannels;
            const unsigned chunk_limit = LoopbackPacket::max_samples / channels;
            const bool valid = !(flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR);
            if (!captureBufferArithmeticValid(position, frames, qpc, valid, format.p->nSamplesPerSec, chunk_limit)) {
              const auto release_result = capture->ReleaseBuffer(frames);
              hr = FAILED(release_result) ? release_result : HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
              break;
            }
            const bool position_discontinuity = device_position.observe(device_pos, frames, valid, (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) != 0);
            if (!frames && position_discontinuity) loss_events.emptyDiscontinuity();
            for (unsigned offset = 0; offset < frames;) {
              const unsigned count = (std::min)(chunk_limit, frames - offset);
              const auto time_offset = static_cast<std::uint64_t>(offset) * 10000000 / format.p->nSamplesPerSec;
              const auto* samples = reinterpret_cast<const float*>(data);
              push(samples ? samples + static_cast<std::size_t>(offset) * channels : nullptr, count, channels, format.p->nSamplesPerSec, epoch,
                   position + offset, valid ? qpc + time_offset : 0, valid, offset == 0 && position_discontinuity, (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0);
              offset += count;
            }
            position += frames;
            hr = capture->ReleaseBuffer(frames);
            if (SUCCEEDED(hr)) hr = capture->GetNextPacketSize(&available);
          }
          if (FAILED(hr)) { fail(hr); break; }
        }
        if (authorization && authorization->cancelled() && SUCCEEDED(hr)) fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
        audio->Stop();
      }
    }
    CoUninitialize();
  }
};
Loopback::Loopback() : impl_(std::make_unique<Impl>()) {}
Loopback::~Loopback() { stop(); }
bool Loopback::start(const std::string& id, std::uint64_t epoch) { return start(id, epoch, {}); }
bool Loopback::start(const std::string& id, std::uint64_t epoch, std::shared_ptr<LoopbackStartAuthorization> authorization) {
  if (authorization && authorization->cancelled()) return false;
  if (impl_->current.load() != State::stopped || impl_->worker.joinable()) return false;
  auto wide = widen(id);
  if (wide.empty()) return false;
  impl_->read = 0; impl_->write = 0; impl_->event_read = 0; impl_->event_write = 0; impl_->event_sequence = 0; impl_->epoch = epoch; impl_->dropped = 0; impl_->error = 0; impl_->cancel = false; impl_->gap = false;
  impl_->current.store(State::starting, std::memory_order_release);
  try { impl_->worker = std::thread(&Impl::run, impl_.get(), std::move(wide), epoch, std::move(authorization)); }
  catch (...) { impl_->current.store(State::stopped); return false; }
  return true;
}
void Loopback::stop() {
  impl_->cancel.store(true, std::memory_order_release);
  if (impl_->worker.joinable()) impl_->worker.join();
  impl_->current.store(State::stopped, std::memory_order_release);
  impl_->read = 0; impl_->write = 0;
  impl_->event_read = 0; impl_->event_write = 0;
}
std::shared_ptr<const LoopbackLossToken> Loopback::lossToken() const noexcept { return impl_->loss; }
Loopback::State Loopback::state() const { return impl_->current.load(std::memory_order_acquire); }
std::int32_t Loopback::error_code() const { return impl_->error.load(std::memory_order_relaxed); }
bool Loopback::pop_event(Event& event) {
  const auto read = impl_->event_read.load(std::memory_order_relaxed);
  if (read == impl_->event_write.load(std::memory_order_acquire)) return false;
  event = impl_->events[read];
  impl_->event_read.store((read + 1) % impl_->events.size(), std::memory_order_release);
  return true;
}
bool Loopback::pop(LoopbackPacket& packet) {
  if (state() != State::running) return false;
  const auto read = impl_->read.load(std::memory_order_relaxed);
  if (read == impl_->write.load(std::memory_order_acquire)) return false;
  packet = impl_->queue[read];
  impl_->read.store((read + 1) % Impl::slots, std::memory_order_release);
  return true;
}
std::uint64_t Loopback::dropped_frames() const { return impl_->dropped.load(std::memory_order_relaxed); }
}
