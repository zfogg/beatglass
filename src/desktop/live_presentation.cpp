#include "live_presentation.hpp"
#include <cmath>
#include <utility>

LivePresentation::LivePresentation(QObject* parent) : QObject(parent) {}
bool LivePresentation::apply(beatglass::PresentationUpdate update) {
  if (!gate_.apply(std::move(update))) return false;
  const auto displayed = gate_.current() ? beatglass::present_snapshot(*gate_.current()) : beatglass::PresentationState{"Stopped", "—", "—", "—", 0, 0, 0, std::nullopt, {}, {}, {}, false};
  status_ = QString::fromStdString(displayed.status);
  tempo_ = QString::fromStdString(displayed.tempo);
  key_ = QString::fromStdString(displayed.key);
  chord_ = QString::fromStdString(displayed.chord);
  beat_count_ = displayed.beat_count;
  observed_frames_ = displayed.observed_frames;
  analysis_delay_frames_ = displayed.analysis_delay_frames;
  confidence_ = displayed.confidence ? *displayed.confidence : -1.0;
  stale_ = displayed.stale;
  tempo_history_.clear();
  for (const auto bpm : displayed.tempo_history) {
    if (std::isfinite(bpm) && bpm > 0.0f) tempo_history_.push_back(QString::number(bpm, 'f', 1));
  }
  tonal_history_.clear();
  for (const auto& label : displayed.tonal_history) tonal_history_.push_back(QString::fromStdString(label));
  emit changed();
  return true;
}
