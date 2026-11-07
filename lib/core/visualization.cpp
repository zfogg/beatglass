#include "beatglass/core/visualization.hpp"
#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace beatglass {
void VisualizationProcessor::reset() noexcept { epoch_ = next_frame_ = window_start_ = 0; sample_rate_ = count_ = 0; }
std::optional<VisualizationFrame> VisualizationProcessor::process(std::uint64_t epoch, std::uint64_t first,
                                                                  std::uint32_t rate, std::span<const float> input) {
  if (!epoch || !rate || input.size() > max_input_frames || input.size() > std::numeric_limits<std::uint64_t>::max() - first ||
      std::any_of(input.begin(), input.end(), [](float x){ return !std::isfinite(x) || x < -1.0f || x > 1.0f; }))
    throw std::invalid_argument("invalid visualization PCM or timing");
  if (epoch_ != epoch || sample_rate_ != rate || next_frame_ != first) {
    reset(); epoch_ = epoch; sample_rate_ = rate; next_frame_ = first;
  }
  std::optional<VisualizationFrame> result;
  constexpr unsigned n = VisualizationFrame::window_size;
  for (float sample : input) {
    if (!count_) window_start_ = next_frame_;
    samples_[count_++] = sample; ++next_frame_;
    if (count_ != n) continue;
    VisualizationFrame frame; frame.epoch = epoch_; frame.first_frame = window_start_; frame.end_frame = next_frame_; frame.sample_rate = rate;
    std::array<std::complex<double>, n> fft{};
    double sum = 0;
    for (unsigned i = 0; i < n; ++i) {
      const double w = 0.5 - 0.5 * std::cos(2 * std::numbers::pi * i / (n - 1));
      sum += w; fft[i] = static_cast<double>(samples_[i]) * w;
    }
    for (unsigned bucket = 0; bucket < frame.waveform_size; ++bucket) {
      auto begin = samples_.begin() + bucket * (n / frame.waveform_size);
      auto [lo, hi] = std::minmax_element(begin, begin + n / frame.waveform_size);
      frame.minimum[bucket] = *lo; frame.maximum[bucket] = *hi;
    }
    for (unsigned i = 1, j = 0; i < n; ++i) {
      unsigned bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit;
      if (i < j) std::swap(fft[i], fft[j]);
    }
    for (unsigned len = 2; len <= n; len <<= 1) {
      const auto step = std::polar(1.0, -2 * std::numbers::pi / len);
      for (unsigned i = 0; i < n; i += len) {
        std::complex<double> w{1, 0};
        for (unsigned j = 0; j < len / 2; ++j) {
          const auto a = fft[i+j], b = fft[i+j+len/2] * w;
          fft[i+j] = a+b; fft[i+j+len/2] = a-b; w *= step;
        }
      }
    }
    for (unsigned k = 0; k < frame.spectrum_size; ++k)
      frame.magnitude[k] = static_cast<float>(std::abs(fft[k]) * ((k == 0 || k == n/2) ? 1.0 : 2.0) / sum);
    result = frame; count_ = 0;
  }
  return result;
}
}


