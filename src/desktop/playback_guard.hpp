#pragma once
#include <QObject>
#include <QString>
#include <cstdint>
class QAudioDevice;
Q_MOC_INCLUDE("QAudioDevice")

/** GUI-thread playback output check for the provisional Qt Multimedia UI. The returned diagnostic describes an absent output or, on Windows with the FFmpeg backend requested, a channel mask that Qt cannot map safely. The caller owns selection and must recheck after device changes. It does not guarantee codec support or audio rendering. */
class PlaybackGuard : public QObject {
  Q_OBJECT
public:
  /** Construct on the GUI thread; an optional parent must share that thread. */
  explicit PlaybackGuard(QObject* parent = nullptr) : QObject(parent) {}
  /** Return true for Qt channel masks that cannot identify usable positions. */
  static bool invalidChannelMask(std::uint32_t mask) noexcept;
  /** Return an empty string if the device passes the applicable checks, otherwise a user-facing diagnostic. */
  Q_INVOKABLE QString diagnostic(const QAudioDevice& device) const;
};
