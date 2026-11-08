#pragma once
#include "beatglass/capture/source_lifecycle.hpp"
#include <QObject>
#include <QString>
/** GUI-thread owner for capture lifecycle status; no audio work occurs here. */
class CaptureMonitorBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString status READ status NOTIFY changed)

 public:
  explicit CaptureMonitorBridge(QObject* parent = nullptr) : QObject(parent) {}
  QString status() const;
  Q_INVOKABLE bool start();
  Q_INVOKABLE void stop();

 signals:
  void changed();

 private:
  beatglass::SourceLifecycle lifecycle_;
};
