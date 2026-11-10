#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include <filesystem>
namespace beatglass::library {
/** Build a worker-owned path decoder operation; verification fails closed when the source is unavailable. */
inline constexpr const char* kFileDecoderSnapshot = "file-reader-v1";
/** Snapshot identity used by the CLI's file analysis pipeline; it remains compatible with the shared file decoder. */
inline constexpr const char* kCliDecoderSnapshot = "cli-aubio-v1";
/** Return whether a persisted decoder snapshot can be reconstructed by this build. */
bool file_decoder_snapshot_compatible(const std::string& snapshot) noexcept;
AnalysisOperation make_file_decode_operation(std::filesystem::path path, AnalysisIdentity identity,
                                             std::string decoder_snapshot = kFileDecoderSnapshot);
}
