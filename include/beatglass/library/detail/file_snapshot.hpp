#pragma once
#include <filesystem>
#include <functional>
#include <cstdint>
#include <memory>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Snapshot creation and verification outcome. */
enum class SnapshotStatus {
  /** Snapshot or source verification succeeded. */ ok,
  /** Stop was observed before completion. */ cancelled,
  /** Source identity, attributes, or contents changed. */ stale_input,
  /** The platform or source cannot supply required evidence. */ unverifiable,
  /** I/O or cryptographic operation failed. */ error
};
/** Private immutable byte snapshot and retained original source, confined to one worker. Provisional POSIX-only implementation; Windows fails closed. Not a complete product integration or an installation API. */
class FileSnapshot {
public:
  /** Release snapshot and original descriptors. A borrowing decoder must already be destroyed. */
  ~FileSnapshot();
  FileSnapshot(const FileSnapshot&) = delete;
  FileSnapshot& operator=(const FileSnapshot&) = delete;
  /** POSIX read-only descriptor borrowed until destruction; it must not be closed or modified by the decoder. */
  int borrowed_descriptor() const noexcept;
  /** Digest of finalized snapshot bytes, prefixed with sha256:. */
  const std::string& digest() const noexcept;
  /** Rehash the retained original and recheck original/path identity. Cancellation is checked between reads. A write or replacement after this check remains possible; unsupported identity evidence fails closed. */
  SnapshotStatus verify(std::stop_token stop = {}) const;
private:
  struct Impl;
  explicit FileSnapshot(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;
  friend struct SnapshotOpen;
};
/** Snapshot creation result; value exists only on success. */
struct SnapshotOpen {
  /** Result category. */ SnapshotStatus status{SnapshotStatus::error};
  /** Owned byte source on success. */ std::unique_ptr<FileSnapshot> value;
  /** Optional failure diagnostic. */ std::string diagnostic;
  /** Create a private snapshot of the source using bounded memory. No original file is written. I/O and the optional byte-progress observer run synchronously on the calling worker and cannot interrupt an in-flight system call. */
  static SnapshotOpen create(const std::filesystem::path& source, std::stop_token stop = {},
                             const std::function<void(std::uintmax_t)>& progress = {});
};
}
