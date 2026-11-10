#include "beatglass/core/tempo_tracker.hpp"
#include <aubio/aubio.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace beatglass {
struct TempoTracker::Impl {
  static constexpr uint_t hop = 512, window = 1024;
  std::uint32_t rate; aubio_tempo_t* tracker = nullptr; fvec_t* input = nullptr; fvec_t* output = nullptr;
  std::uint64_t epoch = 0, next = 0; uint_t used = 0; bool started = false; TempoEstimate result;
  explicit Impl(std::uint32_t r) : rate(r) {
    if (!rate) throw std::invalid_argument("sample rate must be positive");
    input = new_fvec(hop); output = new_fvec(1); tracker = new_aubio_tempo("default", window, hop, rate);
    if (!input || !output || !tracker) { cleanup(); throw std::runtime_error("Could not initialize aubio"); }
  }
  void cleanup() noexcept { if (tracker) del_aubio_tempo(tracker); if (input) del_fvec(input); if (output) del_fvec(output); tracker = nullptr; input = output = nullptr; }
  ~Impl() { cleanup(); }
  void reset() {
    auto* replacement = new_aubio_tempo("default", window, hop, rate);
    if (!replacement) throw std::runtime_error("Could not reset aubio");
    del_aubio_tempo(tracker); tracker = replacement; used = 0; result = {};
  }
};
TempoTracker::TempoTracker(std::uint32_t rate) : impl_(std::make_unique<Impl>(rate)) {}
TempoTracker::~TempoTracker() = default;
void TempoTracker::process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first, bool discontinuity) {
  auto& s = *impl_;
  if (samples.size() > std::numeric_limits<std::uint64_t>::max() - first) throw std::overflow_error("frame position overflow");
  if (s.started && (discontinuity || s.epoch != epoch || s.next != first)) s.reset();
  s.started = true; s.epoch = epoch; s.next = first + samples.size();
  for (float sample : samples) {
    s.input->data[s.used++] = sample;
    if (s.used == Impl::hop) {
      aubio_tempo_do(s.tracker, s.input, s.output); s.used = 0;
      if (s.output->data[0] != 0) {
        ++s.result.beats;
        s.result.beat_frames.push_back(s.next);
        if (s.result.beat_frames.size() > 64) s.result.beat_frames.erase(s.result.beat_frames.begin());
        const auto bpm = aubio_tempo_get_bpm(s.tracker);
        if (std::isfinite(bpm) && bpm > 0) s.result.bpm = bpm;
      }
    }
  }
}
TempoEstimate TempoTracker::estimate() const { return impl_->result; }
}
