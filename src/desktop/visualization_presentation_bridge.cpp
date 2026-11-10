#include "visualization_presentation_bridge.hpp"
#include <utility>
bool VisualizationPresentationBridge::apply(std::uint64_t generation, std::uint64_t revision, const beatglass::VisualizationFrame& frame) noexcept {
  if (!presentation_.accept(generation, revision, frame)) return false;
  QVariantList waveform, spectrum;
  for (unsigned i = 0; i < frame.waveform_size; ++i) waveform.push_back(QVariantList{frame.minimum[i], frame.maximum[i]});
  for (unsigned i = 0; i < beatglass::VisualizationFrame::spectrum_size; ++i)
    spectrum.push_back(frame.magnitude[i]);
  waveform_ = std::move(waveform); spectrum_ = std::move(spectrum); emit changed(); return true;
}
void VisualizationPresentationBridge::clear() noexcept { presentation_.clear(); waveform_.clear(); spectrum_.clear(); emit changed(); }
