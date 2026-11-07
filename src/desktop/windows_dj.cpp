#include "windows_dj.hpp"
#include "platform/windows_endpoints.hpp"
#include "platform/windows_live_tempo.hpp"
#include <QTimer>
#include <QVariantMap>
#include <chrono>
#include <limits>

WindowsDj::WindowsDj(QObject* parent) : QObject(parent), timer_(new QTimer(this)) {
  connect(timer_, &QTimer::timeout, this, &WindowsDj::drain);
  timer_->start(50);
  worker_ = std::thread([this] { run(); });
}
WindowsDj::~WindowsDj() {
  { std::lock_guard lock(mutex_); quitting_ = true; }
  wake_.notify_one();
  if (worker_.joinable()) worker_.join();
}
void WindowsDj::start(const QString& endpoint) {
  if (endpoint.isEmpty()) return;
  bool exhausted = false;
  { std::lock_guard lock(mutex_);
    exhausted = generation_ == std::numeric_limits<std::uint64_t>::max();
    if (!exhausted) { ++generation_; endpoint_ = endpoint.toUtf8().toStdString(); requested_ = true; pending_ = false; }
  }
  if (exhausted) { status_ = QStringLiteral("Cannot start: session limit reached"); emit changed(); return; }
  active_ = true; status_ = QStringLiteral("Starting"); tempo_ = QStringLiteral("\u2014"); emit changed(); wake_.notify_one();
}
void WindowsDj::stop() {
  { std::lock_guard lock(mutex_); if (generation_ != std::numeric_limits<std::uint64_t>::max()) ++generation_; requested_ = false; pending_ = false; }
  active_ = false; status_ = QStringLiteral("Stopped"); tempo_ = QStringLiteral("\u2014"); emit changed(); wake_.notify_one();
}
void WindowsDj::refresh() { { std::lock_guard lock(mutex_); refresh_ = true; } wake_.notify_one(); }
void WindowsDj::drain() {
  Value value;
  { std::lock_guard lock(mutex_); if (!pending_) return; pending_ = false; value = latest_; if (value.generation != generation_) return; }
  sources_ = value.sources; status_ = value.status; active_ = value.active;
  tempo_ = value.bpm ? QString::number(*value.bpm, 'f', 1) : QStringLiteral("\u2014");
  emit changed();
}
void WindowsDj::run() {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::LiveTempo capture(epochs);
  std::uint64_t applied = 0;
  QVariantList sources;
  QString enumerationError;
  bool running = false, appliedRequested = false;
  for (;;) {
    bool requested, enumerate; std::uint64_t generation; std::string endpoint;
    { std::unique_lock lock(mutex_); wake_.wait_for(lock, std::chrono::milliseconds(50));
      if (quitting_) break;
      requested = requested_; enumerate = refresh_; refresh_ = false; generation = generation_; endpoint = endpoint_;
    }
    if (enumerate) {
      const auto result = beatglass::windows::enumerate_render_endpoints();
      sources.clear(); enumerationError = QString::fromStdString(result.error);
      if (result.error.empty()) for (const auto& item : result.endpoints)
        sources.push_back(QVariantMap{{QStringLiteral("name"), QString::fromStdString(item.name)}, {QStringLiteral("id"), QString::fromStdString(item.id)}});
    }
    if (generation != applied || requested != appliedRequested) {
      capture.stop(); running = false; applied = generation; appliedRequested = requested;
      if (requested) running = capture.start(endpoint);
    }
    Value value; value.generation = generation; value.sources = sources; value.active = requested && running;
    if (requested && !running) value.status = QStringLiteral("Capture could not start");
    else if (running && !capture.pump()) { capture.stop(); running = false; value.active = false; value.status = QStringLiteral("Capture interrupted or failed"); }
    else if (running) {
      value.bpm = capture.bpm();
      value.status = capture.state() == beatglass::SourceState::starting ? QStringLiteral("Starting") : (value.bpm ? QStringLiteral("Tracking") : QStringLiteral("Acquiring"));
    } else value.status = enumerationError.isEmpty() ? QStringLiteral("Stopped") : QStringLiteral("Output discovery failed: ") + enumerationError;
    { std::lock_guard lock(mutex_); if (generation == generation_ && requested == requested_) { latest_ = std::move(value); pending_ = true; } }
  }
  capture.stop();
}
