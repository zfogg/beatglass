#include "beatglass/library/analysis_admission.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
namespace beatglass::library {
CacheAdmission admit_analysis(library_detail::SqliteCache& cache, const std::string& run_id, const AnalysisIdentity& identity, const std::string& source_path, const std::string& decoder_snapshot) {
  library_detail::CacheKey key{identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode};
  if (!decoder_snapshot.empty() && !file_decoder_snapshot_compatible(decoder_snapshot)) return {std::nullopt, false, false};
  std::string result;
  if (cache.find(key, result)) return {std::move(result), false};
  cache.begin(run_id, key, source_path, decoder_snapshot);
  return {std::nullopt, true};
}
}
