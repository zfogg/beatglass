#pragma once
#include <cstdint>
#include <string>
namespace beatglass {
/// Provisional job states; transitions and cancellation behavior are not implemented by this type.
enum class JobStatus { queued, running, cancelled, failed, complete };
/// Provisional cache identity fields; hash and version semantics are not yet specified.
struct AnalysisIdentity { std::string content_hash, engine_version, configuration_id, mode; };
/// Provisional frame counts and job status; synchronization and unknown total semantics are unspecified.
struct JobProgress { std::uint64_t frames_processed{}, total_frames{}; JobStatus status{JobStatus::queued}; };
}
