#include "windows_loopback_session.hpp"
#include <utility>

namespace beatglass::windows {
LoopbackSession::LoopbackSession(SourceEpochAllocator& epochs, std::function<void()> invalidate)
    : LoopbackSession(epochs, std::move(invalidate), std::make_unique<Loopback>()) {}
LoopbackSession::LoopbackSession(SourceEpochAllocator& epochs, std::function<void()> invalidate, std::unique_ptr<Loopback> backend)
    : epochs_(epochs), invalidate_(std::move(invalidate)), backend_(std::move(backend)) {}
LoopbackSession::~LoopbackSession() { stop(); }
void LoopbackSession::invalidate() { if (invalidate_) invalidate_(); }
bool LoopbackSession::start(const std::string& endpoint) {
  if (!backend_ || !invalidate_ || lifecycle_.state() != SourceState::stopped) return false;
  auto attempt = lifecycle_.start();
  if (!attempt) return false;
  attempt_ = *attempt;
  auto epoch = epochs_.next();
  if (!epoch) {
    lifecycle_.notify(attempt_, 1, SourceState::error);
    invalidate();
    return false;
  }
  epoch_ = *epoch;
  dropped_ = 0;
  if (!backend_->start(endpoint, epoch_)) {
    lifecycle_.notify(attempt_, 1, SourceState::error);
    invalidate();
    return false;
  }
  return true;
}
void LoopbackSession::poll() {
  Loopback::Event event;
  if (!backend_) return;
  while (backend_->pop_event(event)) {
    if (event.epoch != epoch_) { lifecycle_.delivery_lost(); invalidate(); backend_->stop(); return; }
    const auto next = event.state == Loopback::State::running ? SourceState::running :
        (lifecycle_.state() == SourceState::starting ? SourceState::error : SourceState::interrupted);
    const auto result = lifecycle_.notify(attempt_, event.sequence, next,
        next == SourceState::running ? std::optional<std::uint64_t>(epoch_) : std::nullopt);
    if (result != SourceEventResult::accepted) { lifecycle_.delivery_lost(); invalidate(); backend_->stop(); return; }
    if (next != SourceState::running) invalidate();
  }
  const auto dropped = backend_->dropped_frames();
  if (dropped != dropped_) {
    dropped_ = dropped;
    lifecycle_.delivery_lost();
    invalidate();
    backend_->stop();
  }
}
bool LoopbackSession::pop(LoopbackPacket& packet) {
  poll();
  if (!backend_ || lifecycle_.state() != SourceState::running || backend_->state() != Loopback::State::running) { poll(); return false; }
  if (!backend_->pop(packet)) return false;
  poll();
  if (lifecycle_.state() != SourceState::running || backend_->state() != Loopback::State::running || packet.epoch != epoch_) return false;
  if (packet.discontinuity || !packet.timestamp_valid) invalidate();
  return true;
}
void LoopbackSession::stop() {
  if (lifecycle_.state() != SourceState::stopped) invalidate();
  lifecycle_.stop();
  if (backend_) backend_->stop();
}
SourceState LoopbackSession::state() const { return lifecycle_.state(); }
}
