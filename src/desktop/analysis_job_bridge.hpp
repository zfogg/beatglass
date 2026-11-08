#pragma once
#include "beatglass/library/job_lifecycle.hpp"
#include <QObject>
#include <QString>
/** GUI-thread view of one bounded analysis job lifecycle. */
class AnalysisJobBridge final : public QObject { Q_OBJECT Q_PROPERTY(QString status READ status NOTIFY changed) Q_PROPERTY(qulonglong progress READ progress NOTIFY changed)
public: explicit AnalysisJobBridge(QObject* p=nullptr):QObject(p){} QString status()const; qulonglong progress()const{return job_.snapshot().frames_processed;} Q_INVOKABLE bool start(); Q_INVOKABLE bool cancel(); Q_INVOKABLE bool retry(); Q_INVOKABLE bool complete(); signals:void changed(); private: beatglass::JobLifecycle job_{100}; };
