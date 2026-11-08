#pragma once
#include "beatglass/core/playback_session.hpp"
#include <QObject>
#include <QString>
/** GUI-thread control bridge for the core playback seek/session contract. */
class PlaybackSessionBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(qulonglong epoch READ epoch NOTIFY changed)
  Q_PROPERTY(qulonglong currentFrame READ currentFrame NOTIFY changed)
public:
  explicit PlaybackSessionBridge(QObject* parent = nullptr) : QObject(parent) {}
  QString state() const;
  qulonglong epoch() const noexcept { return session_.epoch(); }
  qulonglong currentFrame() const noexcept { return session_.next_frame(); }
  Q_INVOKABLE bool open(qulonglong epoch);
  Q_INVOKABLE bool pause();
  Q_INVOKABLE void stop();
  Q_INVOKABLE bool seek(qulonglong frame, qulonglong epoch);
signals:
  void changed();
private:
  beatglass::PlaybackSession session_;
};
