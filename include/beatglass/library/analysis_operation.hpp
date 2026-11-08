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
  /** Serialize a completed value for transactional cache publication. */
  std::function<std::string()> serialize;
  /** Revalidate the opened source before the cache commit point. */
  std::function<FileVerificationResult(std::stop_token)> verify;
  /** Monotonic progress in source frames; zero total means unknown. */
  std::function<void(std::uint64_t processed, std::uint64_t total)> progress;
};
}
