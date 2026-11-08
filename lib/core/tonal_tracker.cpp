#include "tonal_tracker.hpp"
#include <essentia.h>
#include <algorithmfactory.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace beatglass::detail {
using essentia::Real;
using Algorithm = essentia::standard::Algorithm;
using Factory = essentia::standard::AlgorithmFactory;
using Algo = std::unique_ptr<Algorithm>;
struct TonalTracker::Impl {
  static constexpr std::size_t frame_size = 4096, hop = 2048, history_size = 32;
  std::uint32_t rate;
  Algo window, spectrum, peaks, hpcp, key, chord;
  std::array<float, frame_size> frame{};
  std::size_t used{};
  std::uint64_t epoch{}, next{};
  bool started{};
  std::vector<std::vector<Real>> history;
  TonalEstimate result;
  explicit Impl(std::uint32_t r) : rate(r) {
    if (r < 12000) throw std::invalid_argument("sample rate must be at least 12000 Hz");
    static std::once_flag initialized;
    std::call_once(initialized, [] { essentia::init(); });
    auto& f = Factory::instance();
    window.reset(f.create("Windowing", "type", "hann"));
    spectrum.reset(f.create("Spectrum"));
    peaks.reset(f.create("SpectralPeaks", "sampleRate", static_cast<Real>(r)));
    hpcp.reset(f.create("HPCP", "size", 12));
    key.reset(f.create("Key", "profileType", "bgate"));
    chord.reset(f.create("Key", "profileType", "tonictriad", "usePolyphony", false));
    history.reserve(history_size);
  }
  void reset() { used = 0; history.clear(); result = {}; }
  static void classify(Algorithm& algo, std::vector<Real>& pcp, std::string& name, Real& strength) {
    std::string note, scale; Real separation{};
    algo.input("pcp").set(pcp); algo.output("key").set(note); algo.output("scale").set(scale);
    algo.output("strength").set(strength); algo.output("firstToSecondRelativeStrength").set(separation);
    algo.compute();
    if (scale != "minor" && scale != "major") throw std::runtime_error("unsupported Essentia scale");
    name = note + (scale == "minor" ? " minor" : " major");
  }
  void analyze(std::uint64_t boundary) {
    double power{};
    for (float sample : frame) power += static_cast<double>(sample) * sample;
    result = {}; result.observed_from = boundary - frame_size; result.observed_through = boundary;
    if (power / frame_size < 1e-8f) { history.clear(); return; }
    std::vector<Real> input(frame.begin(), frame.end()), windowed, spec, freq, mag, chroma;
    window->input("frame").set(input); window->output("frame").set(windowed);
    spectrum->input("frame").set(windowed); spectrum->output("spectrum").set(spec);
    peaks->input("spectrum").set(spec); peaks->output("frequencies").set(freq); peaks->output("magnitudes").set(mag);
    hpcp->input("frequencies").set(freq); hpcp->input("magnitudes").set(mag); hpcp->output("hpcp").set(chroma);
    window->compute(); spectrum->compute(); peaks->compute();
    if (freq.empty()) { history.clear(); return; }
    hpcp->compute();
    if (chroma.empty() || !std::all_of(chroma.begin(), chroma.end(), [](Real x) { return std::isfinite(x); }) ||
        *std::max_element(chroma.begin(), chroma.end()) <= 0) { history.clear(); return; }
    if (history.size() == history_size) history.erase(history.begin());
    history.push_back(chroma);
    result.key_from = result.observed_from - (history.size() - 1) * hop;
    std::vector<Real> average(chroma.size());
    for (const auto& h : history) for (std::size_t i = 0; i < h.size(); ++i) average[i] += h[i] / history.size();
    std::string key_name, chord_name; Real ks{}, cs{};
    // Key and chord are independent evidence streams. A malformed or
    // unsupported chord result must not erase a valid track-level key (and
    // vice versa); each label is published only when its own score is usable.
    try { classify(*key, average, key_name, ks); } catch (...) { key_name.clear(); ks = 0; }
    try { classify(*chord, chroma, chord_name, cs); } catch (...) { chord_name.clear(); cs = 0; }
    // Essentia's strengths are uncalibrated evidence, not probabilities. A
    // non-positive or non-finite score is no evidence at all; fail closed so
    // the UI cannot present a confident-looking label for an ambiguous frame.
    if (!key_name.empty() && std::isfinite(ks) && ks > 0) {
      result.key = key_name; result.key_strength = ks;
    }
    if (!chord_name.empty() && std::isfinite(cs) && cs > 0) {
      result.chord = chord_name; result.chord_strength = cs;
    }
  }
};
TonalTracker::TonalTracker(std::uint32_t rate) : impl_(std::make_unique<Impl>(rate)) {}
TonalTracker::~TonalTracker() = default;
void TonalTracker::process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first, bool discontinuity) {
  auto& s = *impl_;
  if (samples.size() > std::numeric_limits<std::uint64_t>::max() - first) {
    s.reset(); s.started = false; throw std::overflow_error("frame position overflow");
  }
  if (!std::all_of(samples.begin(), samples.end(), [](float x) { return std::isfinite(x); })) {
    s.reset(); s.started = false; throw std::invalid_argument("non-finite PCM");
  }
  if (s.started && (discontinuity || s.epoch != epoch || s.next != first)) s.reset();
  s.started = true; s.epoch = epoch;
  try {
    for (std::size_t i = 0; i < samples.size(); ++i) {
      s.frame[s.used++] = samples[i];
      if (s.used == Impl::frame_size) {
        s.analyze(first + i + 1);
        std::move(s.frame.begin() + Impl::hop, s.frame.end(), s.frame.begin()); s.used = Impl::hop;
      }
    }
    s.next = first + samples.size();
  } catch (...) { s.reset(); s.started = false; throw; }
}
TonalEstimate TonalTracker::estimate() const { return impl_->result; }
}
