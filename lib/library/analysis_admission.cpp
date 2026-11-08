#include "beatglass/library/analysis_admission.hpp"
namespace beatglass::library {
CacheAdmission admit_analysis(library_detail::SqliteCache& cache, const std::string& run_id, const AnalysisIdentity& identity) {
  library_detail::CacheKey key{identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode};
  std::string result;
  if (cache.find(key, result)) return {std::move(result), false};
  cache.begin(run_id, key);
  return {std::nullopt, true};
}
}
