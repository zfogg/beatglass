#pragma once
#include "beatglass/library/store.hpp"
#include <cstdint>
#include <functional>
#include <stop_token>
#include <string>
namespace beatglass::library {
/** Worker-owned analysis operation contract; implementations perform real DSP and never run on the audio callback. */
struct AnalysisOperation {
  FileVersion source;
  AnalysisIdentity identity;
  /** Decoder implementation/configuration snapshot retained for persisted retries. */
  std::string decoder_snapshot;
  /** Serialize a completed value for transactional cache publication. */
  std::function<std::string()> serialize;
  /** Optional stop-aware decoder/analysis step; progress is monotonic source frames. */
  std::function<std::string(std::stop_token, const std::function<void(std::uint64_t)>&)> decode;
  /** Receive the verified serialized value on the worker before the job completes. The callback must not touch GUI or audio state. */
  std::function<void(const std::string&)> publish;
  /** Stop-aware transactional publication; return false when commit is cancelled or rejected. */
  std::function<bool(const std::string&, std::stop_token)> publish_transaction;
  /** Recover a reserved persistent run when cancellation interrupts work before publication. */
  std::function<void()> recover_cancellation;
  /** Revalidate the opened source before the cache commit point. */
  std::function<FileVerificationResult(std::stop_token)> verify;
  /** Monotonic progress in source frames; zero total means unknown. */
  std::function<void(std::uint64_t processed, std::uint64_t total)> progress;
  /** Return whether source attribution, bounded NUL-free identity fields, supported mode, and required callbacks are complete. A worker must provide either decode or serialize. */
  bool valid() const noexcept { return !source.path.empty() && source.path.string().find('\0') == std::string::npos && (decoder_snapshot.empty() || (decoder_snapshot.find('\0') == std::string::npos && decoder_snapshot.size() <= 4096)) && !source.content_hash.empty() && source.content_hash.find('\0') == std::string::npos && source.content_hash.size() <= 512 && source.content_hash == identity.content_hash && !identity.engine_version.empty() && identity.engine_version.find('\0') == std::string::npos && identity.engine_version.size() <= 4096 && !identity.configuration_id.empty() && identity.configuration_id.find('\0') == std::string::npos && identity.configuration_id.size() <= 4096 && (identity.mode == "offline" || identity.mode == "live") && static_cast<bool>(serialize) != static_cast<bool>(decode) && static_cast<bool>(verify); }
};
}
