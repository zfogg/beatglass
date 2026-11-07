#include "windows_dj_bridge.hpp"
#include <QMetaType>
#include <chrono>
#include <limits>
#include <utility>

namespace beatglass::windows {
DjBridge::DjBridge(QObject* parent, std::function<EndpointResult()> discovery) : QObject(parent) {
  qRegisterMetaType<PresentationUpdate>();
  connect(&timer_, &QTimer::timeout, this, &DjBridge::drain);
  timer_.start(50);
  worker_ = std::thread(&DjBridge::run, this, std::move(discovery));
}
DjBridge::~DjBridge() {
  timer_.stop();
  { std::lock_guard lock(mutex_); quitting_ = true; }
  wake_.notify_one();
  if (worker_.joinable()) worker_.join();
}
void DjBridge::start() {
  bool exhausted;
  {
    std::lock_guard lock(mutex_);
    if (requested_) return;
    exhausted = commands_exhausted_ || request_generation_ == std::numeric_limits<std::uint64_t>::max();
    if (exhausted) { commands_exhausted_ = true; requested_ = false; reset_pending_ = true; }
    else { requested_ = true; ++request_generation_; }
  }
  if (exhausted) status_ = QStringLiteral("Control sequence exhausted");
  wake_.notify_one(); emit stateChanged();
}
void DjBridge::stop() {
  {
    std::lock_guard lock(mutex_);
    requested_ = false; reset_pending_ = true;
    if (pending_ && pending_->snapshot) pending_.reset();
    if (request_generation_ == std::numeric_limits<std::uint64_t>::max()) commands_exhausted_ = true;
    else ++request_generation_;
  }
  wake_.notify_one(); emit stateChanged();
}
bool DjBridge::active() const { std::lock_guard lock(mutex_); return requested_; }
QString DjBridge::status() const { return status_; }
void DjBridge::drain() {
  std::optional<PresentationUpdate> update;
  std::optional<std::string> status;
  {
    std::lock_guard lock(mutex_);
    update = std::move(pending_); pending_.reset();
    if (have_status_) { status = std::move(pending_status_); have_status_ = false; }
  }
  if (update) emit presentationUpdate(std::move(*update));
  if (status) {
    auto next = QString::fromStdString(*status);
    if (next != status_) { status_ = std::move(next); emit stateChanged(); }
  }
}
void DjBridge::run(std::function<EndpointResult()> discovery) {
  SourceEpochAllocator epochs;
  DefaultTempoFollower follower(epochs, std::move(discovery));
  bool active = false;
  bool fatal = false;
  for (;;) {
    bool requested, quitting, reset, exhausted;
    std::uint64_t generation;
    {
      std::unique_lock lock(mutex_);
      wake_.wait_for(lock, std::chrono::milliseconds(50));
      requested = requested_; quitting = quitting_; reset = reset_pending_; reset_pending_ = false; generation = request_generation_; exhausted = commands_exhausted_;
    }
    if (quitting) break;
    if (reset) { follower.stop(); active = false; fatal = false; }
    bool ok = true;
    if (requested != active) {
      if (requested) { fatal = false; if (!follower.start()) { requested = false; ok = false; } }
      else follower.stop();
      active = requested;
    }
    if (active && ok) ok = follower.tick(std::chrono::steady_clock::now());
    if (!ok) fatal = true;
    auto update = follower.takeUpdate();
    const auto state = follower.status();
    std::string text = exhausted ? "Control sequence exhausted" : fatal ? "Capture or presentation error" : !active ? "Stopped" : state == DefaultTempoFollower::Status::waiting ? "Waiting for default output" :
        state == DefaultTempoFollower::Status::starting ? "Starting" : state == DefaultTempoFollower::Status::running ? "Monitoring default output" :
        follower.diagnostic().empty() ? "Capture error" : follower.diagnostic();
    {
      std::lock_guard lock(mutex_);
      if (!ok) { if (generation == request_generation_) requested_ = false; active = false; }
      if (update && (!update->snapshot || (requested_ && generation == request_generation_))) pending_ = std::move(update);
      pending_status_ = std::move(text); have_status_ = true;
    }
  }
  follower.stop();
}
}
