#pragma once
#include <cstdint>
#include <string>
namespace beatglass {
enum class JobStatus { queued, running, cancelled, failed, complete };
struct AnalysisIdentity { std::string content_hash, engine_version, configuration_id, mode; };
struct JobProgress { std::uint64_t frames_processed{}, total_frames{}; JobStatus status{JobStatus::queued}; };
}
