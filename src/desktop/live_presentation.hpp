#pragma once
#include "beatglass/core/presentation_gate.hpp"
#include <QObject>
#include <QString>

/**
 * GUI-thread-owned presentation of revisioned immutable live snapshots.
 *
 * A producer must deliver updates to this object's thread in accordance with
 * PresentationGate's revision and lifetime contract. apply() does not queue
 * across threads. A clear removes all displayed estimates; recreating this
 * object requires draining old updates or guarding them with a session ID.
 * Only tracking snapshots expose estimates; all other states clear labels.
 * This provisional adapter does not start capture or expire stale data itself.
 */
class LivePresentation : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(QString tempo READ tempo NOTIFY changed)
  Q_PROPERTY(QString key READ key NOTIFY changed)
  Q_PROPERTY(QString chord READ chord NOTIFY changed)
  Q_PROPERTY(qulonglong beatCount READ beatCount NOTIFY changed)
  Q_PROPERTY(qulonglong observedFrames READ observedFrames NOTIFY changed)
  Q_PROPERTY(qulonglong analysisDelayFrames READ analysisDelayFrames NOTIFY changed)
  Q_PROPERTY(double confidence READ confidence NOTIFY changed)
  Q_PROPERTY(bool stale READ stale NOTIFY changed)
public:
  /** Construct on the owning GUI thread; parent, if any, has the same thread. */
  explicit LivePresentation(QObject* parent = nullptr);
  /** Apply a newer value or clear. Returns false for an older or duplicate revision. */
  bool apply(beatglass::PresentationUpdate update);
  /** Current tracking state label; initially and after clear, "Stopped". */
  QString status() const { return status_; }
  /** Current BPM, or an em dash if unavailable. */
  QString tempo() const { return tempo_; }
  /** First key candidate label, or an em dash if unavailable. */
  QString key() const { return key_; }
  /** Most recent chord label, or an em dash if unavailable. */
  QString chord() const { return chord_; }
  /** Number of beats in the accepted snapshot. */
  qulonglong beatCount() const { return beat_count_; }
  /** Exclusive captured frame boundary in the accepted snapshot. */
  qulonglong observedFrames() const { return observed_frames_; }
  /** Published analysis delay measured in frames. */
  qulonglong analysisDelayFrames() const { return analysis_delay_frames_; }
  /** Signal-health confidence in [0, 1], or -1 when unavailable. */
  double confidence() const { return confidence_; }
  /** True when the current state is not providing fresh tracking evidence. */
  bool stale() const { return stale_; }
signals:
  /** Emitted on the owning thread when a newer update is applied. */
  void changed();
private:
  beatglass::PresentationGate gate_;
  QString status_ = QStringLiteral("Stopped");
  QString tempo_ = QStringLiteral("\u2014"), key_ = QStringLiteral("\u2014"), chord_ = QStringLiteral("\u2014");
  qulonglong beat_count_ = 0, observed_frames_ = 0, analysis_delay_frames_ = 0;
  double confidence_ = -1.0;
  bool stale_ = false;
};
