#include "beatglass/core/visualization.hpp"
#include <kfr/dft.hpp>
#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace beatglass {
VisualizationProcessor::VisualizationProcessor(Invalidate invalidate, void* context) : invalidate_(invalidate), context_(context) {
  if (!invalidate_) throw std::invalid_argument("visualization invalidation hook required");
}
void VisualizationProcessor::clear_partial() noexcept { epoch_ = next_frame_ = window_start_ = 0; sample_rate_ = count_ = 0; }
void VisualizationProcessor::reset() noexcept { invalidate_(context_); clear_partial(); }
std::optional<VisualizationFrame> VisualizationProcessor::process(std::uint64_t epoch, std::uint64_t first,
                                                                  std::uint32_t rate, std::span<const float> input, bool discontinuity) {
  if (!epoch || !rate || input.size() > max_input_frames || input.size() > std::numeric_limits<std::uint64_t>::max() - first ||
      std::any_of(input.begin(), input.end(), [](float x){ return !std::isfinite(x) || x < -1.0f || x > 1.0f; }))
    { reset(); throw std::invalid_argument("invalid visualization PCM or timing"); }
  if (epoch < high_epoch_ || (epoch == high_epoch_ && (first < high_next_ || rate != high_rate_))) {
    reset(); throw std::invalid_argument("stale visualization PCM");
  }
  if (epoch > high_epoch_) { high_epoch_ = epoch; high_next_ = first; high_rate_ = rate; }
  if (discontinuity || epoch_ != epoch || sample_rate_ != rate || next_frame_ != first) {
    if (epoch_ || discontinuity) reset(); else clear_partial(); epoch_ = epoch; sample_rate_ = rate; next_frame_ = first;
  }
  std::optional<VisualizationFrame> result;
  constexpr unsigned n = VisualizationFrame::window_size;
  for (float sample : input) {
    if (!count_) window_start_ = next_frame_;
    samples_[count_++] = sample; ++next_frame_; high_next_ = next_frame_;
    if (count_ != n) continue;
    VisualizationFrame frame; frame.epoch = epoch_; frame.first_frame = window_start_; frame.end_frame = next_frame_; frame.sample_rate = rate;
    kfr::dft_plan_real<float> fft_plan(n, kfr::dft_pack_format::CCs);
    kfr::univector<float, n> fft_input;
    kfr::univector<kfr::complex<float>> fft_output(fft_plan.complex_size());
    kfr::univector<kfr::u8> fft_scratch(fft_plan.temp_size);
    double sum = 0;
    for (unsigned i = 0; i < n; ++i) {
      const double w = 0.5 - 0.5 * std::cos(2 * std::numbers::pi * i / (n - 1));
      sum += w; fft_input[i] = static_cast<float>(samples_[i] * w);
    }
    for (unsigned bucket = 0; bucket < frame.waveform_size; ++bucket) {
      auto begin = samples_.begin() + bucket * (n / frame.waveform_size);
      auto [lo, hi] = std::minmax_element(begin, begin + n / frame.waveform_size);
      frame.minimum[bucket] = *lo; frame.maximum[bucket] = *hi;
    }
    fft_plan.execute(fft_output, fft_input, fft_scratch.data());
    for (unsigned k = 0; k < frame.spectrum_size; ++k)
      frame.magnitude[k] = std::abs(fft_output[k]) * static_cast<float>((k == 0 || k == n/2) ? 1.0 : 2.0) / static_cast<float>(sum);
    result = frame; count_ = 0;
  }
  return result;
}
}


