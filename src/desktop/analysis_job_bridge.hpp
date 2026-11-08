#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include <QObject>
class AnalysisJobBridge final : public QObject { Q_OBJECT Q_PROPERTY(qulonglong frames READ frames NOTIFY progressChanged) public: explicit AnalysisJobBridge(beatglass::library_detail::JobExecutor& executor,QObject* parent=nullptr):QObject(parent),runner_(executor){} std::shared_ptr<beatglass::library_detail::JobHandle> submit(beatglass::library::AnalysisOperation op); qulonglong frames()const{return frames_;} signals:void progressChanged(); private: beatglass::library::AnalysisRunner runner_; qulonglong frames_{}; };
