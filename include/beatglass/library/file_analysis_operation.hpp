#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include <filesystem>
namespace beatglass::library {
/** Build a provisional worker-owned path decoder operation; no GUI/audio I/O. */
AnalysisOperation make_file_decode_operation(std::filesystem::path path, AnalysisIdentity identity);
}
