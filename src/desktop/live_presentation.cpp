#include "live_presentation.hpp"
#include <utility>

LivePresentation::LivePresentation(QObject* parent) : QObject(parent) {}
bool LivePresentation::apply(beatglass::PresentationUpdate update) {
  if (!gate_.apply(std::move(update))) return false;
  const auto displayed = gate_.current() ? beatglass::present_snapshot(*gate_.current()) : beatglass::PresentationState{"Stopped", "—", "—", "—", false};
  status_ = QString::fromStdString(displayed.status);
  tempo_ = QString::fromStdString(displayed.tempo);
  key_ = QString::fromStdString(displayed.key);
  chord_ = QString::fromStdString(displayed.chord);
  emit changed();
  return true;
}
