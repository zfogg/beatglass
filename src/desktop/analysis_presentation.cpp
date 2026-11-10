#include "analysis_presentation.hpp"
#include <cmath>

void AnalysisPresentation::publish(const beatglass::Snapshot& snapshot) {
  const auto displayed = beatglass::present_snapshot(snapshot);
  bpm_ = QString::fromStdString(displayed.tempo);
  key_ = QString::fromStdString(displayed.key);
  chord_ = QString::fromStdString(displayed.chord);
  confidence_ = displayed.confidence ? *displayed.confidence : -1.0;
  stale_ = displayed.stale;
  beats_ = displayed.beat_count;
  frames_ = displayed.observed_frames;
  tempo_history_.clear();
  for (const auto bpm : displayed.tempo_history) {
    if (std::isfinite(bpm) && bpm > 0.0f) tempo_history_.push_back(QString::fromStdString(beatglass::format_presentation_bpm(bpm)));
  }
  tonal_history_.clear();
  for (const auto& label : displayed.tonal_history) tonal_history_.push_back(QString::fromStdString(label));
  beat_frames_.clear();
  for (const auto frame : displayed.beat_frames) beat_frames_.push_back(QVariant::fromValue<qulonglong>(frame));
  emit changed();
}
