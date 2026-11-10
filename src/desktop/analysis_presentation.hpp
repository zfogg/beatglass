#pragma once
#include "beatglass/core/presentation.hpp"
#include <QObject>
#include <QStringList>
#include <QVariantList>
class AnalysisPresentation final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString bpm READ bpm NOTIFY changed)
  Q_PROPERTY(QString key READ key NOTIFY changed)
  Q_PROPERTY(QString chord READ chord NOTIFY changed)
  Q_PROPERTY(double confidence READ confidence NOTIFY changed)
  Q_PROPERTY(bool stale READ stale NOTIFY changed)
  Q_PROPERTY(qulonglong beatCount READ beatCount NOTIFY changed)
  Q_PROPERTY(qulonglong observedFrames READ observedFrames NOTIFY changed)
  Q_PROPERTY(qulonglong analysisDelayFrames READ analysisDelayFrames NOTIFY changed)
  Q_PROPERTY(QStringList tempoHistory READ tempoHistory NOTIFY changed)
  Q_PROPERTY(QStringList tonalHistory READ tonalHistory NOTIFY changed)
  Q_PROPERTY(QVariantList beatFrames READ beatFrames NOTIFY changed)

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
  /** Published analysis delay measured in source frames. */
  qulonglong analysisDelayFrames() const { return analysis_delay_frames_; }
  /** Recent valid tempo estimates from the shared causal presentation. */
  QStringList tempoHistory() const { return tempo_history_; }
  /** Recent chord labels from the shared causal presentation. */
  QStringList tonalHistory() const { return tonal_history_; }
  /** Current-epoch causal beat frame positions from the shared presentation. */
  QVariantList beatFrames() const { return beat_frames_; }

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
  qulonglong analysis_delay_frames_{};
  QStringList tempo_history_;
  QStringList tonal_history_;
  QVariantList beat_frames_;
};
