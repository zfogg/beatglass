#include "live_presentation.hpp"
#include <cmath>
#include <utility>

LivePresentation::LivePresentation(QObject* parent) : QObject(parent) {}
bool LivePresentation::apply(beatglass::PresentationUpdate update) {
  if (!gate_.apply(std::move(update))) return false;
  status_ = QStringLiteral("Stopped");
  tempo_ = key_ = chord_ = QStringLiteral("\u2014");
  if (const auto snapshot = gate_.current()) {
    switch (snapshot->state) {
      case beatglass::TrackingState::stopped: status_ = QStringLiteral("Stopped"); break;
      case beatglass::TrackingState::silent: status_ = QStringLiteral("Silent"); break;
      case beatglass::TrackingState::acquiring: status_ = QStringLiteral("Acquiring"); break;
      case beatglass::TrackingState::tracking: status_ = QStringLiteral("Tracking"); break;
      case beatglass::TrackingState::reacquiring: status_ = QStringLiteral("Reacquiring"); break;
      case beatglass::TrackingState::error: status_ = QStringLiteral("Error"); break;
    }
    if (snapshot->state != beatglass::TrackingState::tracking) { emit changed(); return true; }
    if (snapshot->bpm && std::isfinite(*snapshot->bpm) && *snapshot->bpm > 0)
      tempo_ = QString::number(*snapshot->bpm, 'f', 1);
    if (!snapshot->key_candidates.empty() && !snapshot->key_candidates.front().label.empty())
      key_ = QString::fromStdString(snapshot->key_candidates.front().label);
    if (!snapshot->chords.empty() && !snapshot->chords.back().chord.label.empty())
      chord_ = QString::fromStdString(snapshot->chords.back().chord.label);
  }
  emit changed();
  return true;
}
