#include "windows_default_tempo.hpp"
#include <limits>
#include <memory>
#include <utility>

namespace beatglass::windows {
DefaultTempoFollower::DefaultTempoFollower(SourceEpochAllocator& epochs, std::function<EndpointResult()> discovery, std::function<bool(const std::function<bool()>&)> startGuard)
    : tempo_(epochs), discovery_(std::move(discovery)), start_guard_(std::move(startGuard)) {
  if (!discovery_) discovery_ = enumerate_render_endpoints;
}
bool DefaultTempoFollower::start() {
  if (exhausted_) return false;
  enabled_ = true; next_probe_ = {};
  return true;
}
bool DefaultTempoFollower::nextRevision() {
  if (revision_ == std::numeric_limits<std::uint64_t>::max()) { exhausted_ = true; pending_.reset(); published_bpm_.reset(); tempo_.stop(); endpoint_.clear(); status_ = Status::error; diagnostic_ = "Presentation revision exhausted"; return false; }
  ++revision_; return true;
}
bool DefaultTempoFollower::publishClear() {
  published_bpm_.reset();
  if (!nextRevision()) return false;
  pending_ = StampedTempoUpdate{PresentationUpdate{revision_, {}}, {}, {}};
  return true;
}
bool DefaultTempoFollower::publishTempo(float bpm, EventTime observed) {
  if (!nextRevision()) return false;
  Snapshot snapshot;
  snapshot.state = TrackingState::tracking;
  snapshot.bpm = bpm;
  snapshot.observed_through = observed;
  snapshot.publication_monotonic_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  snapshot.estimator_version = "aubio-tempo";
  snapshot.configuration_id = "windows-live-tempo-experimental";
  const auto token = tempo_.lossToken(); const auto stamp = tempo_.lossStamp();
  if (!token || !stamp || !token->matches(*stamp)) return publishClear();
  pending_ = StampedTempoUpdate{PresentationUpdate{revision_, std::make_shared<const Snapshot>(std::move(snapshot))}, token, stamp};
  published_bpm_ = bpm;
  return true;
}
bool DefaultTempoFollower::tick(std::chrono::steady_clock::time_point now) {
  if (exhausted_) return false;
  if (!enabled_) return true;
  try {
  if (!endpoint_.empty() && !tempo_.pump()) {
    if (!publishClear()) return false;
    tempo_.stop(); endpoint_.clear();
    status_ = Status::error; diagnostic_ = "Capture interrupted or analysis failed";
    next_probe_ = now + std::chrono::seconds(1);
  }
  if (!endpoint_.empty() && tempo_.state() == SourceState::running) { status_ = Status::running; diagnostic_.clear(); }
  if (tempo_.invalidationGeneration() != last_invalidation_) {
    last_invalidation_ = tempo_.invalidationGeneration();
    if (published_bpm_ && !publishClear()) return false;
  }
  if (now >= next_probe_) {
    next_probe_ = now + std::chrono::seconds(1);
    const auto result = discovery_();
    std::string selected;
    bool ambiguous = false;
    if (result.error.empty()) for (const auto& item : result.endpoints) if (item.is_default) {
      if (!selected.empty()) ambiguous = true;
      selected = item.id;
    }
    if (!result.error.empty() || ambiguous) selected.clear();
    if (selected != endpoint_) {
      if (!publishClear()) return false;
      tempo_.stop(); endpoint_.clear();
      if (!selected.empty()) {
        const auto start = [&] { return tempo_.start(selected); };
        if (start_guard_ ? start_guard_(start) : start()) { endpoint_ = std::move(selected); status_ = Status::starting; diagnostic_.clear(); }
        else { status_ = Status::error; diagnostic_ = "Capture start failed"; }
      }
    }
    if (endpoint_.empty() && selected.empty()) {
      status_ = (!result.error.empty() || ambiguous) ? Status::error : Status::waiting;
      diagnostic_ = ambiguous ? "Multiple default endpoints returned" : result.error;
    }
  }
  const auto bpm = tempo_.bpm();
  const auto observed = tempo_.observedThrough();
  if (bpm && observed) {
    if (!published_bpm_ || *published_bpm_ != *bpm) return publishTempo(*bpm, *observed);
  } else if (published_bpm_) return publishClear();
  return true;
  } catch (...) {
    publishClear();
    tempo_.stop(); endpoint_.clear(); enabled_ = false;
    status_ = Status::error;
    return false;
  }
}
std::optional<StampedTempoUpdate> DefaultTempoFollower::takeUpdate() { auto result = std::move(pending_); pending_.reset(); return result; }
void DefaultTempoFollower::stop() { enabled_ = false; if (!exhausted_) publishClear(); tempo_.stop(); endpoint_.clear(); next_probe_ = {}; if (!exhausted_) { status_ = Status::waiting; diagnostic_.clear(); } }
const std::string& DefaultTempoFollower::endpoint() const { return endpoint_; }
DefaultTempoFollower::Status DefaultTempoFollower::status() const { return status_; }
const std::string& DefaultTempoFollower::diagnostic() const { return diagnostic_; }
}
