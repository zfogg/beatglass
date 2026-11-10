#include "live_presentation.hpp"
#include <utility>

LivePresentation::LivePresentation(QObject* parent) : QObject(parent) {}
bool LivePresentation::apply(beatglass::PresentationUpdate update) {
  if (!gate_.apply(std::move(update))) return false;
  const auto displayed = gate_.current() ? beatglass::present_snapshot(*gate_.current()) : beatglass::PresentationState{"Stopped", "—", "—", "—", 0, 0, 0, false};
  status_ = QString::fromStdString(displayed.status);
  tempo_ = QString::fromStdString(displayed.tempo);
  key_ = QString::fromStdString(displayed.key);
  chord_ = QString::fromStdString(displayed.chord);
  beat_count_ = displayed.beat_count;
  observed_frames_ = displayed.observed_frames;
  analysis_delay_frames_ = displayed.analysis_delay_frames;
  emit changed();
  return true;
}
