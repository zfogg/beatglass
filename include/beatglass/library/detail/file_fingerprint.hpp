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
/** SHA-256 evidence for the bytes observed by one read, with file attributes around that read. It is not an opened-decoder identity or immunity to concurrent mutation. */
struct FileFingerprint {
  /** Outcome; only ok permits use of remaining fields. */ FingerprintStatus status{FingerprintStatus::error};
  /** Number of bytes hashed. */ std::uintmax_t size{};
  /** Filesystem modification time on the implementation's file clock. */ std::filesystem::file_time_type modified{};
  /** Lowercase SHA-256 hex, prefixed with sha256:. */ std::string digest;
  /** Diagnostic when the operation fails. */ std::string diagnostic;
};
/** Hash a file on the calling worker using a bounded buffer; never writes the file. The observer, if present, runs synchronously after each chunk and may request stop. Blocking I/O cannot be interrupted. Attributes and digest cannot exclude all replacement, mutation, or ABA races; do not use this alone to prove that a separately opened decoder analyzed these bytes. */
FileFingerprint fingerprint_file(const std::filesystem::path& path, std::stop_token stop = {},
                                 const std::function<void(std::uintmax_t)>& observer = {});
}
