#include "beat_tracker.hpp"
#include <aubio/aubio.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
namespace beatglass::detail {
struct BeatTracker::Impl {
  static constexpr uint_t hop = 512, window = 1024;
  std::uint32_t rate;
  aubio_tempo_t* tracker{}; fvec_t* input{}; fvec_t* output{};
  std::uint64_t epoch{}, origin{}, next{}; uint_t used{}; bool started{}, failed{};
  std::optional<std::uint64_t> pending;
  explicit Impl(std::uint32_t r) : rate(r) {
    if (!rate) throw std::invalid_argument("sample rate must be positive");
    input = new_fvec(hop); output = new_fvec(1); tracker = new_aubio_tempo("default", window, hop, rate);
    if (!input || !output || !tracker) { cleanup(); throw std::runtime_error("Could not initialize aubio"); }
  }
  void cleanup() noexcept { if (tracker) del_aubio_tempo(tracker); if (input) del_fvec(input); if (output) del_fvec(output); tracker = nullptr; input = output = nullptr; }
  ~Impl() { cleanup(); }
  void reset(std::uint64_t e, std::uint64_t first) {
    auto* replacement = new_aubio_tempo("default", window, hop, rate);
    if (!replacement) { failed = true; throw std::runtime_error("Could not reset aubio"); }
    del_aubio_tempo(tracker); tracker = replacement; epoch = e; origin = next = first;
    used = 0; pending.reset(); started = true;
  }
};
BeatTracker::BeatTracker(std::uint32_t rate) : impl_(std::make_unique<Impl>(rate)) {}
BeatTracker::~BeatTracker() = default;
void BeatTracker::process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first,
                          const Observer& observer, bool discontinuity) {
  auto& s = *impl_;
  if (s.failed) throw std::runtime_error("Beat tracker failed");
  if (samples.size() > std::numeric_limits<std::uint64_t>::max() - first) { s.failed = true; s.pending.reset(); throw std::overflow_error("frame position overflow"); }
  if (!std::all_of(samples.begin(), samples.end(), [](float x) { return std::isfinite(x); })) { s.failed = true; s.pending.reset(); throw std::invalid_argument("non-finite PCM"); }
  if (!s.started || discontinuity || s.epoch != epoch || s.next != first) s.reset(epoch, first);
  constexpr std::uint64_t max_frames = (static_cast<std::uint64_t>(std::numeric_limits<uint_t>::max()) / Impl::hop) * Impl::hop;
  if (samples.size() > max_frames - (s.next - s.origin)) { s.failed = true; s.pending.reset(); throw std::overflow_error("aubio frame counter exhausted; new tracker and epoch required"); }
  try {
    for (float sample : samples) {
      s.input->data[s.used++] = sample; ++s.next;
      if (s.pending && *s.pending < s.next) {
        if (observer) observer({s.epoch, *s.pending, s.next});
        s.pending.reset();
      }
      if (s.used != Impl::hop) continue;
      aubio_tempo_do(s.tracker, s.input, s.output); s.used = 0;
      if (s.output->data[0] == 0) continue;
      const auto local = static_cast<std::uint64_t>(aubio_tempo_get_last(s.tracker));
      const auto observed = s.next - s.origin;
      if (local > observed) throw std::runtime_error("aubio returned future beat");
      const auto frame = s.origin + local;
      if (frame == s.next) s.pending = frame;
      else if (observer) observer({s.epoch, frame, s.next});
    }
  } catch (...) { s.failed = true; s.pending.reset(); throw; }
}
}
