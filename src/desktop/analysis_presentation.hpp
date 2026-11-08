#pragma once
#include "beatglass/core/results.hpp"
#include <QObject>
class AnalysisPresentation final : public QObject { Q_OBJECT Q_PROPERTY(QString bpm READ bpm NOTIFY changed) Q_PROPERTY(QString key READ key NOTIFY changed) Q_PROPERTY(qulonglong beatCount READ beatCount NOTIFY changed) Q_PROPERTY(qulonglong observedFrames READ observedFrames NOTIFY changed) public: using QObject::QObject; void publish(const beatglass::Snapshot&); QString bpm() const{return bpm_;} QString key() const{return key_;} qulonglong beatCount()const{return beats_;} qulonglong observedFrames()const{return frames_;} signals:void changed(); private:QString bpm_{QStringLiteral("—")},key_{QStringLiteral("—")};qulonglong beats_{},frames_{};};
