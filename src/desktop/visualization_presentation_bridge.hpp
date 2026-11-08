#pragma once
#include "beatglass/core/visualization_presentation.hpp"
#include <QObject>
#include <QVariantList>
/** GUI-thread bridge for an accepted bounded visualization snapshot. */
class VisualizationPresentationBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList waveform READ waveform NOTIFY changed)
  Q_PROPERTY(QVariantList spectrum READ spectrum NOTIFY changed)
public:
  using QObject::QObject;
  /** Apply an immutable worker frame; stale or malformed frames are rejected. */
  bool apply(std::uint64_t generation, std::uint64_t revision, const beatglass::VisualizationFrame& frame) noexcept;
  /** Clear the painted frame and advance its source generation. */
  Q_INVOKABLE void clear() noexcept;
  QVariantList waveform() const { return waveform_; }
  QVariantList spectrum() const { return spectrum_; }
signals: void changed();
private:
  beatglass::VisualizationPresentation presentation_;
  QVariantList waveform_, spectrum_;
};
