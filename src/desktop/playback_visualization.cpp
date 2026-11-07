#include "playback_visualization.hpp"
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioFormat>
#include <QObject>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
constexpr unsigned maxFrames = 4096, maxChannels = 8, queueCapacity = 8;
constexpr qint64 maxTimestampUs = 7LL * 24 * 60 * 60 * 1000000;
constexpr std::size_t maxBytes = std::size_t(maxFrames) * maxChannels * sizeof(float);
using Status = PlaybackVisualization::Status;
struct Chunk {
  std::array<std::byte, maxBytes> bytes{};
  unsigned frames{}, channels{}, rate{};
  qint64 startUs{-1};
  QAudioFormat::SampleFormat format{QAudioFormat::Unknown};
};
struct Shared {
  std::array<Chunk, queueCapacity> queue{};
  std::atomic<unsigned> head{0}, tail{0};
  std::atomic<unsigned> state{static_cast<unsigned>(Status::active)};
  std::atomic_flag producer = ATOMIC_FLAG_INIT;
  std::mutex latestMutex;
  std::optional<PlaybackVisualization::Frame> latest;
  std::atomic<unsigned> publishedRevision{0};
  std::atomic<unsigned> acceptedRevision{0};
  std::uint64_t epoch{};
  void stop(Status reason) noexcept {
    unsigned expected = static_cast<unsigned>(Status::active);
    state.compare_exchange_strong(expected, static_cast<unsigned>(reason), std::memory_order_acq_rel);
  }
  bool active() const noexcept { return state.load(std::memory_order_acquire) == static_cast<unsigned>(Status::active); }
  void enqueue(const QAudioBuffer& buffer) noexcept {
    if (!active()) return;
    if (producer.test_and_set(std::memory_order_acquire)) { stop(Status::overflow); return; }
    struct Clear { std::atomic_flag& flag; ~Clear() { flag.clear(std::memory_order_release); } } clear{producer};
    if (!active()) return;
    if (!buffer.isValid() || buffer.frameCount() == 0) { stop(Status::end_of_stream); return; }
    const auto format = buffer.format();
    const auto sample = format.sampleFormat();
    const int channels = format.channelCount(), rate = format.sampleRate(), bytes = format.bytesPerSample();
    const auto frames = buffer.frameCount();
    const auto startUs = buffer.startTime();
    if (channels <= 0 || channels > int(maxChannels) || rate <= 0 || rate > 384000 || startUs < 0 || startUs > maxTimestampUs || frames <= 0 || frames > maxFrames ||
        (sample != QAudioFormat::UInt8 && sample != QAudioFormat::Int16 && sample != QAudioFormat::Int32 && sample != QAudioFormat::Float) ||
        (bytes != 1 && bytes != 2 && bytes != 4)) { stop(Status::unsupported_audio); return; }
    const std::size_t size = std::size_t(frames) * unsigned(channels) * unsigned(bytes);
    if (size > maxBytes || buffer.byteCount() < qsizetype(size) || !buffer.constData<void>()) { stop(Status::unsupported_audio); return; }
    const auto write = head.load(std::memory_order_relaxed);
    if (write - tail.load(std::memory_order_acquire) >= queueCapacity) { stop(Status::overflow); return; }
    auto& chunk = queue[write % queueCapacity];
    chunk.frames = unsigned(frames); chunk.channels = unsigned(channels); chunk.rate = unsigned(rate); chunk.format = sample; chunk.startUs = startUs;
    std::memcpy(chunk.bytes.data(), buffer.constData<void>(), size);
    head.store(write + 1, std::memory_order_release);
  }
};
static_assert(std::atomic<unsigned>::is_always_lock_free);
float sampleAt(const Chunk& c, std::size_t index) {
  switch (c.format) {
    case QAudioFormat::UInt8: return (float(std::to_integer<unsigned>(c.bytes[index])) - 128.0f) / 128.0f;
    case QAudioFormat::Int16: { std::int16_t x; std::memcpy(&x, c.bytes.data() + index * 2, 2); return float(x) / 32768.0f; }
    case QAudioFormat::Int32: { std::int32_t x; std::memcpy(&x, c.bytes.data() + index * 4, 4); return float(double(x) / 2147483648.0); }
    case QAudioFormat::Float: { float x; std::memcpy(&x, c.bytes.data() + index * 4, 4); return x; }
    default: return std::numeric_limits<float>::quiet_NaN();
  }
}
void run(const std::shared_ptr<Shared>& shared, std::stop_token stop) {
  beatglass::VisualizationProcessor processor([](void* p) noexcept { static_cast<Shared*>(p)->stop(Status::unsupported_audio); }, shared.get());
  std::array<float, maxFrames> mono{};
  std::uint64_t position = 0;
  unsigned rate = 0, channels = 0;
  qint64 anchorUs = -1;
  QAudioFormat::SampleFormat format = QAudioFormat::Unknown;
  while (!stop.stop_requested() && shared->active()) {
    const auto read = shared->tail.load(std::memory_order_relaxed);
    if (read == shared->head.load(std::memory_order_acquire)) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }
    const auto& chunk = shared->queue[read % queueCapacity];
    if (rate && (rate != chunk.rate || channels != chunk.channels || format != chunk.format)) { shared->stop(Status::unsupported_audio); break; }
    rate = chunk.rate; channels = chunk.channels; format = chunk.format;
    if (anchorUs < 0) anchorUs = chunk.startUs;
    const auto expectedUs = static_cast<long double>(anchorUs) + static_cast<long double>(position) * 1000000.0L / rate;
    if (std::fabs(static_cast<long double>(chunk.startUs) - expectedUs) > 2.0L) { shared->stop(Status::unsupported_audio); break; }
    bool bad = false;
    for (unsigned i = 0; i < chunk.frames; ++i) {
      double sum = 0;
      for (unsigned ch = 0; ch < channels; ++ch) { const auto value = sampleAt(chunk, std::size_t(i) * channels + ch); if (!std::isfinite(value)) bad = true; sum += value; }
      mono[i] = float(std::clamp(sum / channels, -1.0, 1.0));
    }
    if (bad || chunk.frames > std::numeric_limits<std::uint64_t>::max() - position) { shared->stop(Status::unsupported_audio); break; }
    try {
      for (unsigned first = 0; first < chunk.frames; first += beatglass::VisualizationFrame::window_size) {
        const auto count = std::min<unsigned>(beatglass::VisualizationFrame::window_size, chunk.frames - first);
        auto result = processor.process(shared->epoch, position, rate, std::span<const float>(mono.data() + first, count));
        position += count;
        if (result && shared->active()) {
          std::lock_guard lock(shared->latestMutex);
          if (shared->active()) {
            const auto previous = shared->publishedRevision.load(std::memory_order_relaxed);
            if (previous >= std::numeric_limits<unsigned>::max() - 1) { shared->stop(Status::overflow); break; }
            const auto revision = previous + 1;
            shared->latest = PlaybackVisualization::Frame{*result, revision};
            shared->publishedRevision.store(revision, std::memory_order_release);
          }
        }
      }
    } catch (...) { shared->stop(Status::unsupported_audio); break; }
    shared->tail.store(read + 1, std::memory_order_release);
  }
  shared->stop(Status::retired);
}
}
struct PlaybackVisualization::Impl {
  std::shared_ptr<Shared> state;
  std::unique_ptr<QAudioBufferOutput> output;
  std::jthread worker;
  explicit Impl(std::uint64_t epoch) : state(std::make_shared<Shared>()), output(std::make_unique<QAudioBufferOutput>()) {
    state->epoch = epoch;
    QObject::connect(output.get(), &QAudioBufferOutput::audioBufferReceived, output.get(), [state = state](const QAudioBuffer& buffer) { state->enqueue(buffer); }, Qt::DirectConnection);
    worker = std::jthread([state = state](std::stop_token stop) { try { run(state, stop); } catch (...) { state->stop(Status::unsupported_audio); } });
  }
  ~Impl() { state->stop(Status::retired); worker.request_stop(); if (worker.joinable()) worker.join(); }
};
PlaybackVisualization::PlaybackVisualization(std::uint64_t epoch) {
  if (!epoch) throw std::invalid_argument("playback visualization requires a nonzero epoch");
  impl_ = std::make_unique<Impl>(epoch);
}
PlaybackVisualization::~PlaybackVisualization() = default;
QAudioBufferOutput* PlaybackVisualization::output() const noexcept { return impl_->output.get(); }
void PlaybackVisualization::retire() noexcept { impl_->state->stop(Status::retired); }
PlaybackVisualization::Status PlaybackVisualization::status() const noexcept { return static_cast<Status>(impl_->state->state.load(std::memory_order_acquire)); }
unsigned PlaybackVisualization::revision() const noexcept {
  return impl_->state->active() ? impl_->state->publishedRevision.load(std::memory_order_acquire) : std::numeric_limits<unsigned>::max();
}
bool PlaybackVisualization::acceptFrame(std::uint64_t epoch, unsigned revision) noexcept {
  auto& state = *impl_->state;
  if (!revision || revision == std::numeric_limits<unsigned>::max() || epoch != state.epoch || !state.active() ||
      revision > state.publishedRevision.load(std::memory_order_acquire) || revision <= state.acceptedRevision.load(std::memory_order_relaxed)) return false;
  state.acceptedRevision.store(revision, std::memory_order_release);
  return state.active();
}
bool PlaybackVisualization::isCurrent(std::uint64_t epoch, unsigned revision) const noexcept {
  return revision != 0 && revision != std::numeric_limits<unsigned>::max() && impl_->state->epoch == epoch && impl_->state->active() && impl_->state->acceptedRevision.load(std::memory_order_acquire) == revision && impl_->state->active();
}
std::optional<PlaybackVisualization::Frame> PlaybackVisualization::takeLatest() {
  if (!impl_->state->active()) return {};
  std::lock_guard lock(impl_->state->latestMutex);
  if (!impl_->state->active()) return {};
  auto result = impl_->state->latest; impl_->state->latest.reset(); return result;
}
