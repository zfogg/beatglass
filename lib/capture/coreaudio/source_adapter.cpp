#include "beatglass/capture/coreaudio_source_internal.hpp"
#include <utility>

namespace beatglass::coreaudio {
namespace {
SourceError source_error(const TapStatus& status) {
  if (!status.failure) return SourceError::backend_error;
  switch (*status.failure) {
    case TapFailure::invalid_argument: return SourceError::not_found;
    case TapFailure::not_found: return SourceError::not_found;
    case TapFailure::permission_denied: return SourceError::permission_denied;
    case TapFailure::unsupported: return SourceError::unsupported;
    case TapFailure::device_lost: return SourceError::device_lost;
    default: return SourceError::backend_error;
  }
}
bool same_format(const AudioFormat& a, const CaptureBlock& b) {
  return a.sample_rate == b.sample_rate && a.channels == b.channels && b.sample_count &&
         b.sample_count <= CaptureBlock::max_samples && b.channels && b.sample_count % b.channels == 0;
}
}
SourceAdapter::SourceAdapter(SourceEpochAllocator& epochs, std::unique_ptr<SourceBackend> backend)
    : epochs_(epochs), backend_(std::move(backend)) {}
SourceAdapter::~SourceAdapter() { (void)stop(); }
EnumerationResult SourceAdapter::enumerate() {
  if (!backend_) return {EnumerationOutcome::error, {}, SourceError::backend_error, "Core Audio backend unavailable"};
  try {
    auto result = backend_->enumerate();
    if (result.outcome == EnumerationOutcome::error) {
      result.sources.clear();
      if (!result.error) result.error = SourceError::backend_error;
      if (result.diagnostic.empty()) result.diagnostic = "Core Audio output enumeration failed";
    } else if (result.outcome == EnumerationOutcome::ok && result.sources.empty() && result.diagnostic.empty()) {
      result.error.reset();
      result.diagnostic = "No Core Audio output devices available";
    } else if (result.outcome == EnumerationOutcome::ok && !result.sources.empty()) {
      result.error.reset();
      result.diagnostic.clear();
    } else if (result.outcome == EnumerationOutcome::cancelled && result.diagnostic.empty()) {
      result.sources.clear();
      result.error = SourceError::cancelled;
      result.diagnostic = "Core Audio output enumeration cancelled";
    } else if (result.outcome == EnumerationOutcome::cancelled) {
      result.sources.clear();
      result.error = SourceError::cancelled;
      result.diagnostic = "Core Audio output enumeration cancelled";
    }
    return result;
  }
  catch (...) { return {EnumerationOutcome::error, {}, SourceError::backend_error, "Core Audio enumeration failed"}; }
}
SourceStartResult SourceAdapter::start(const std::string& id, AudioSource::Consumer consumer) {
  if (!backend_) return {{}, SourceError::backend_error, "Core Audio backend unavailable"};
  if (!consumer) return {{}, SourceError::backend_error, "Audio consumer required"};
  if (id.empty() || id.find('\0') != std::string::npos || id.find_first_not_of(" \t\r\n") == std::string::npos)
    return {{}, SourceError::not_found, "Output source id required"};
  if (quiescence_failed_) return {{}, SourceError::backend_error, "Previous capture has not quiesced"};
  if (lifecycle_.state() != SourceState::stopped) return {{}, SourceError::busy, "Source already active"};
  auto epoch = epochs_.next();
  if (!epoch) return {{}, SourceError::backend_error, "Source epoch exhausted"};
  auto attempt = lifecycle_.start();
  if (!attempt) return {{}, SourceError::backend_error, "Source attempt exhausted"};
  TapStatus failure;
  bool native_started = false;
  try {
    native_started = backend_->start(id, *epoch);
    if (!native_started) failure = backend_->status();
  } catch (...) {
    failure = {TapState::error, "Core Audio startup threw", TapFailure::backend_error};
  }
  if (!native_started) {
    const bool stopped = backend_->stop(); quiescence_failed_ = !stopped;
    lifecycle_.stop();
    const auto error = source_error(failure);
    std::string diagnostic = failure.message.empty() ? "Core Audio capture start failed" : failure.message;
    if (!stopped) diagnostic += "; backend stop also failed";
    status_ = {SourceState::error, error, std::move(diagnostic)};
    return {{}, error, status_.message};
  }
  std::optional<AudioFormat> format;
  bool format_query_failed = false;
  try { format = backend_->format(); }
  catch (...) { format.reset(); format_query_failed = true; }
  if (!format || !format->sample_rate || !format->channels) {
    const bool stopped = backend_->stop(); quiescence_failed_ = !stopped; lifecycle_.stop();
    std::string diagnostic = format_query_failed ? "Core Audio format query failed" : "Invalid capture format";
    if (!stopped) diagnostic += "; backend stop also failed";
    status_ = {SourceState::error, SourceError::backend_error, std::move(diagnostic)};
    return {{}, SourceError::backend_error, status_.message};
  }
  attempt_ = attempt; format_ = format; consumer_ = std::move(consumer); sequence_ = 1;
  running_taken_ = running_acknowledged_ = false; anchor_.reset();
  if (lifecycle_.notify(*attempt, sequence_, SourceState::running, *epoch) != SourceEventResult::accepted) {
    (void)stop(); return {{}, SourceError::backend_error, "Invalid lifecycle transition"};
  }
  status_ = {SourceState::running, {}, {}};
  pending_ = SourceEvent{*attempt, sequence_, SourceState::running, epoch, format_, status_};
  return {attempt, {}, {}};
}
bool SourceAdapter::stop() noexcept {
  if (delivering_) {
    // A stop request from the borrowed-block callback is rejected while the
    // adapter is still delivering.  The callback does not own lifecycle
    // teardown, so refusing the request must not fabricate an error state or
    // invalidate the running epoch underneath the active pump.
    return false;
  }
  lifecycle_.stop(); pending_.reset(); attempt_.reset(); format_.reset(); anchor_.reset();
  running_taken_ = running_acknowledged_ = false; consumer_ = {};
  const bool stopped = !backend_ || backend_->stop();
  quiescence_failed_ = !stopped;
  status_.state = stopped ? SourceState::stopped : SourceState::error;
  status_.error = stopped ? std::optional<SourceError>{} : SourceError::backend_error;
  status_.message = stopped ? std::string{} : "Core Audio backend failed to quiesce";
  return stopped;
}
SourceStatus SourceAdapter::status() const { return status_; }
void SourceAdapter::interrupt(const TapStatus& native) {
  if (!attempt_ || lifecycle_.state() != SourceState::running) return;
  ++sequence_;
  const auto result = lifecycle_.notify(*attempt_, sequence_, SourceState::interrupted);
  anchor_.reset(); running_acknowledged_ = false;
  if (result != SourceEventResult::accepted) {
    lifecycle_.delivery_lost(); status_ = {SourceState::error, SourceError::backend_error, "Source lifecycle ordering failed"};
  } else status_ = {SourceState::interrupted, source_error(native), native.message};
  pending_ = SourceEvent{*attempt_, sequence_, status_.state, {}, {}, status_};
}
std::optional<SourceEvent> SourceAdapter::take_event() {
  if (delivering_) return std::nullopt;
  if (!pending_ && lifecycle_.state() == SourceState::running) {
    try {
      const auto current = backend_->status();
      if (current.state != TapState::running) interrupt(current);
    } catch (...) { interrupt({TapState::error, "Core Audio status threw", TapFailure::backend_error}); }
  }
  auto result = std::move(pending_); pending_.reset();
  if (result && result->state == SourceState::running) running_taken_ = true;
  return result;
}
bool SourceAdapter::acknowledge_running(SourceAttempt attempt, std::uint64_t sequence) noexcept {
  if (delivering_ || !running_taken_ || !attempt_ || attempt_->generation != attempt.generation || sequence != 1 ||
      lifecycle_.state() != SourceState::running) return false;
  running_acknowledged_ = true; return true;
}
bool SourceAdapter::pump() {
  if (delivering_ || !running_acknowledged_ || pending_ || lifecycle_.state() != SourceState::running) return false;
  try {
  auto current = backend_->status();
  if (current.state != TapState::running) { interrupt(current); return false; }
  if (!backend_->pop(block_)) {
    current = backend_->status();
    if (current.state != TapState::running) interrupt(current);
    return false;
  }
  current = backend_->status();
  if (current.state != TapState::running) { interrupt(current); return false; }
  const auto ns = block_.host_time_valid ? backend_->host_time_ns(block_.host_time) : std::nullopt;
  if (!format_ || !lifecycle_.epoch() || block_.epoch != *lifecycle_.epoch() || !same_format(*format_, block_) || !ns) {
    interrupt({TapState::interrupted, "Invalid epoch, format, PCM, or unavailable host timestamp", TapFailure::backend_error});
    return false;
  }
  anchor_ = AudioClockAnchor{block_.epoch, block_.first_frame, *ns, AudioClockDomain::backend_monotonic, {}, {}};
  const AudioBlock borrowed{*format_, block_.epoch, block_.first_frame, *ns, block_.discontinuity,
                            std::span<const float>(block_.samples.data(), block_.sample_count)};
  delivering_ = true;
  struct DeliveryDone { bool& active; ~DeliveryDone() { active = false; } } delivery_done{delivering_};
  try { consumer_(borrowed); }
  catch (...) { interrupt({TapState::interrupted, "PCM consumer threw", TapFailure::backend_error}); return false; }
  return true;
  } catch (...) {
    interrupt({TapState::error, "Core Audio polling threw", TapFailure::backend_error});
    return false;
  }
}
std::optional<AudioClockAnchor> SourceAdapter::latest_anchor() const noexcept { return anchor_; }
}
