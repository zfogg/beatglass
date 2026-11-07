#include "windows_dj.hpp"
#include "platform/windows_endpoints.hpp"
#include "platform/windows_live_tempo.hpp"
#include "platform/windows_default_tempo.hpp"
#include <QTimer>
#include <QVariantMap>
#include <chrono>
#include <limits>

WindowsDj::WindowsDj(beatglass::SourceEpochAllocator& epochs, QObject* parent) : WindowsDj(epochs, nullptr, {}, parent) {}
WindowsDj::WindowsDj(beatglass::SourceEpochAllocator& epochs, std::unique_ptr<beatglass::windows::LiveTempo> selectedCapture, std::function<beatglass::windows::EndpointResult()> discovery, QObject* parent) : QObject(parent), epochs_(epochs), timer_(new QTimer(this)) {
  connect(timer_, &QTimer::timeout, this, &WindowsDj::drain);
  timer_->start(50);
  worker_ = std::thread([this, capture = std::move(selectedCapture), discovery = std::move(discovery)]() mutable { run(std::move(capture), std::move(discovery)); });
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
  if (sources_ != value.sources) { sources_ = value.sources; emit sourcesChanged(); }
  const auto tempo = value.bpm ? QString::number(*value.bpm, 'f', 1) : QStringLiteral("\u2014");
  if (status_ != value.status || active_ != value.active || tempo_ != tempo) {
    status_ = value.status; active_ = value.active; tempo_ = tempo; emit changed();
  }
}
void WindowsDj::run(std::unique_ptr<beatglass::windows::LiveTempo> capture, std::function<beatglass::windows::EndpointResult()> discovery) {
  if (!capture) capture = std::make_unique<beatglass::windows::LiveTempo>(epochs_);
  std::uint64_t applied = 0;
  beatglass::windows::DefaultTempoFollower follower(epochs_, std::move(discovery), [this, &applied](const std::function<bool()>& start) {
    std::lock_guard lock(mutex_);
    if (quitting_ || !requested_ || generation_ != applied || endpoint_ != "__beatglass_default__") return false;
    return start();
  });
  std::optional<float> followedBpm;
  bool following = false;
  QVariantList sources;
  QString enumerationError;
  bool running = false, appliedRequested = false;
  bool everRunning = false, recovering = false;
  std::chrono::steady_clock::time_point retryAt{};
  auto startSelectedIfCurrent = [&](std::uint64_t generation, const std::string& endpoint) {
    std::lock_guard lock(mutex_);
    if (quitting_ || !requested_ || generation != generation_ || endpoint != endpoint_) return false;
    return capture->start(endpoint);
  };
  for (;;) {
    bool requested, enumerate; std::uint64_t generation; std::string endpoint;
    { std::unique_lock lock(mutex_); wake_.wait_for(lock, std::chrono::milliseconds(50));
      if (quitting_) break;
      requested = requested_; enumerate = refresh_; refresh_ = false; generation = generation_; endpoint = endpoint_;
    }
    if (enumerate) {
      const auto result = beatglass::windows::enumerate_render_endpoints();
      sources.clear(); enumerationError = QString::fromStdString(result.error);
      sources.push_back(QVariantMap{{QStringLiteral("name"), QStringLiteral("System default (follow changes)")}, {QStringLiteral("id"), QStringLiteral("__beatglass_default__")}});
      if (result.error.empty()) for (const auto& item : result.endpoints)
        sources.push_back(QVariantMap{{QStringLiteral("name"), QString::fromStdString(item.name)}, {QStringLiteral("id"), QString::fromStdString(item.id)}});
    }
    if (generation != applied || requested != appliedRequested) {
      capture->stop(); follower.stop(); followedBpm.reset(); running = false; everRunning = false; recovering = false;
      applied = generation; appliedRequested = requested;
      following = endpoint == "__beatglass_default__";
      if (requested) running = following ? follower.start() : startSelectedIfCurrent(generation, endpoint);
    }
    if (requested && !following && recovering && !running && std::chrono::steady_clock::now() >= retryAt) {
      capture->stop(); running = startSelectedIfCurrent(generation, endpoint);
      if (!running) retryAt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    }
    Value value; value.generation = generation; value.sources = sources; value.active = requested && (running || recovering);
    if (requested && !running) value.status = recovering ? QStringLiteral("Output interrupted; retrying") : QStringLiteral("Capture could not start");
    else if (running && following) {
      if (!follower.tick(std::chrono::steady_clock::now())) {
        followedBpm.reset(); running = false; value.active = false; value.status = QStringLiteral("Capture interrupted or failed");
      } else {
        if (const auto update = follower.takeUpdate()) followedBpm = update->snapshot ? update->snapshot->bpm : std::nullopt;
        value.bpm = followedBpm;
        using Status = beatglass::windows::DefaultTempoFollower::Status;
        switch (follower.status()) {
          case Status::waiting: value.status = QStringLiteral("Waiting for default output"); break;
          case Status::starting: value.status = QStringLiteral("Starting"); break;
          case Status::running: value.status = value.bpm ? QStringLiteral("Tracking") : QStringLiteral("Acquiring"); break;
          case Status::error: value.status = QStringLiteral("Capture error: ") + QString::fromStdString(follower.diagnostic()); break;
        }
      }
    } else if (running && !capture->pump()) {
      capture->stop(); running = false; recovering = everRunning;
      if (recovering) retryAt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
      value.active = requested && recovering;
      value.status = recovering ? QStringLiteral("Output interrupted; retrying") : QStringLiteral("Capture interrupted or failed");
    }
    else if (running) {
      if (capture->state() == beatglass::SourceState::running) { everRunning = true; recovering = false; }
      value.bpm = capture->bpm();
      value.status = capture->state() == beatglass::SourceState::starting ? QStringLiteral("Starting") : (value.bpm ? QStringLiteral("Tracking") : QStringLiteral("Acquiring"));
    } else value.status = enumerationError.isEmpty() ? QStringLiteral("Stopped") : QStringLiteral("Output discovery failed: ") + enumerationError;
    { std::lock_guard lock(mutex_); if (generation == generation_ && requested == requested_) { latest_ = std::move(value); pending_ = true; } }
  }
  capture->stop(); follower.stop();
}
