#pragma once
#include "beatglass/core/playback_session.hpp"
#include <QObject>
#include <QString>
#include <QUrl>
#include <QStringList>
/** GUI-thread control bridge for the core playback seek/session contract. */
class PlaybackSessionBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY changed)
  Q_PROPERTY(qulonglong epoch READ epoch NOTIFY changed)
  Q_PROPERTY(qulonglong currentFrame READ currentFrame NOTIFY changed)
  Q_PROPERTY(QUrl source READ source NOTIFY changed)
  Q_PROPERTY(QStringList devices READ devices NOTIFY changed)
  Q_PROPERTY(int deviceId READ deviceId WRITE setDeviceId NOTIFY changed)
public:
  explicit PlaybackSessionBridge(QObject* parent = nullptr) : QObject(parent) {}
  QString state() const;
  qulonglong epoch() const noexcept { return session_.epoch(); }
  qulonglong currentFrame() const noexcept { return session_.next_frame(); }
  QUrl source() const { return source_; }
  QStringList devices() const { return devices_; }
  int deviceId() const noexcept { return device_id_; }
  void setDeviceId(int id) noexcept { if (device_id_ != id) { device_id_ = id; emit changed(); } }
  Q_INVOKABLE void refreshDevices();
  Q_INVOKABLE bool open(qulonglong epoch);
  Q_INVOKABLE bool openSource(const QUrl& source, qulonglong epoch);
  Q_INVOKABLE bool replaceSource(const QUrl& source, qulonglong epoch);
  Q_INVOKABLE bool pause();
  Q_INVOKABLE void stop();
  Q_INVOKABLE bool seek(qulonglong frame, qulonglong epoch);
signals:
  void changed();
private:
  beatglass::PlaybackSession session_;
  QUrl source_;
  QStringList devices_;
  int device_id_{-1};
};
