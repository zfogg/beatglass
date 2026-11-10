#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Outcome of computing provisional file-content evidence. */
enum class FingerprintStatus {
  /** All bytes were read and pre/post attributes agreed. */ ok,
  /** Cancellation was observed while reading. */ cancelled,
  /** File size or modification time changed during the read. */ changed,
  /** The file could not be read or the digest failed. */ error
};
/** Stable user-facing label for a fingerprint outcome. */
inline const char* fingerprint_status_label(FingerprintStatus status) noexcept {
  switch (status) {
    case FingerprintStatus::ok: return "ok";
    case FingerprintStatus::cancelled: return "cancelled";
    case FingerprintStatus::changed: return "changed";
    case FingerprintStatus::error: return "error";
  }
  return "error";
}
/** Return whether an outcome can be retried without changing the source. */
inline bool fingerprint_status_is_retryable(FingerprintStatus status) noexcept {
  return status == FingerprintStatus::cancelled || status == FingerprintStatus::error;
}
/** Return whether cached identity must be refreshed before another attempt. */
inline bool fingerprint_status_requires_refresh(FingerprintStatus status) noexcept {
  return status == FingerprintStatus::changed;
}
/** Combine a stable status label with optional diagnostic detail. */
inline std::string fingerprint_status_diagnostic(FingerprintStatus status,
                                                 const std::string& detail = {}) {
  const std::string label = fingerprint_status_label(status);
  return detail.empty() ? label : label + ": " + detail;
}
/** SHA-256 evidence for the bytes observed by one read, with file attributes around that read. It is not an opened-decoder identity or immunity to concurrent mutation. */
struct FileFingerprint {
  /** Outcome; only ok permits use of remaining fields. */ FingerprintStatus status{FingerprintStatus::error};
  /** Number of bytes hashed. */ std::uintmax_t size{};
  /** Filesystem modification time on the implementation's file clock. */ std::filesystem::file_time_type modified{};
  /** Lowercase SHA-256 hex, prefixed with sha256:. */ std::string digest;
  /** Diagnostic when the operation fails. */ std::string diagnostic;
};
/** Hash a file on the calling worker using a bounded buffer; never writes the file.
 * The observer, if present, runs synchronously after each nonempty chunk and may
 * request stop; cancellation is cooperative and cannot interrupt blocking I/O.
 * Observer and filesystem exceptions derived from std::exception return error
 * with a diagnostic; other exceptions propagate. Input longer than the initial
 * size, a short input, or changed attributes return changed; I/O errors return
 * error. Attributes and digest cannot exclude all replacement, mutation, or ABA
 * races; they do not prove that a separately opened decoder analyzed these bytes.
 */
FileFingerprint fingerprint_file(const std::filesystem::path& path, std::stop_token stop = {},
                                 const std::function<void(std::uintmax_t)>& observer = {});
}
