#include "beatglass/library/detail/cache_identity.hpp"
namespace beatglass::library_detail {
CacheIdentityResult cache_identity_from_file(const std::filesystem::path& path,
                                             std::string engine,
                                             std::string config,
                                             std::string mode,
                                             std::stop_token stop) {
  CacheIdentityResult out;
  const auto fingerprint = fingerprint_file(path, stop);
  out.status = fingerprint.status;
  out.diagnostic = fingerprint.diagnostic;
  if (fingerprint.status != FingerprintStatus::ok) return out;
  out.key = CacheKey{fingerprint.digest, std::move(engine), std::move(config), std::move(mode)};
  return out;
}
}
