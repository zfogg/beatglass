#pragma once
#include "beatglass/core/presentation.hpp"
#include <QObject>
class AnalysisPresentation final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString bpm READ bpm NOTIFY changed)
  Q_PROPERTY(QString key READ key NOTIFY changed)
  Q_PROPERTY(QString chord READ chord NOTIFY changed)
  Q_PROPERTY(double confidence READ confidence NOTIFY changed)
  Q_PROPERTY(bool stale READ stale NOTIFY changed)
  Q_PROPERTY(qulonglong beatCount READ beatCount NOTIFY changed)
  Q_PROPERTY(qulonglong observedFrames READ observedFrames NOTIFY changed)

 public:
  using QObject::QObject;
  void publish(const beatglass::Snapshot&);
  QString bpm() const { return bpm_; }
  QString key() const { return key_; }
  QString chord() const { return chord_; }
  double confidence() const { return confidence_; }
  bool stale() const { return stale_; }
  qulonglong beatCount() const { return beats_; }
  qulonglong observedFrames() const { return frames_; }

 signals:
  void changed();

 private:
  QString bpm_{QStringLiteral("—")};
  QString key_{QStringLiteral("—")};
  QString chord_{QStringLiteral("—")};
  double confidence_{-1.0};
  bool stale_{};
  qulonglong beats_{};
  qulonglong frames_{};
};
