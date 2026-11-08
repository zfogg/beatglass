#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include <filesystem>
namespace beatglass::library {
/** Build a worker-owned path decoder operation; verification fails closed when the source is unavailable. */
inline constexpr const char* kFileDecoderSnapshot = "file-reader-v1";
AnalysisOperation make_file_decode_operation(std::filesystem::path path, AnalysisIdentity identity,
                                             std::string decoder_snapshot = kFileDecoderSnapshot);
}
