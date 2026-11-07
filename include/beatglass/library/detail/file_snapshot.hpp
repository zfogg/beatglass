#pragma once
#include <filesystem>
#include <functional>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <cstddef>
#include <stop_token>
#include <string>
#include <string_view>
namespace beatglass::library_detail {
#ifdef _WIN32
/** Classify resolved Windows path, drive type and filesystem evidence for the restricted policy. This is necessary but not sufficient evidence of source safety. */
bool windows_restricted_volume_supported(std::wstring_view resolved_path, unsigned drive_type, std::wstring_view filesystem) noexcept;
#endif
/** Source sharing policy for a provisional snapshot. */
enum class SnapshotSourcePolicy {
  detect_changes, ///< Allow concurrent opens and detect changes with evidence checks.
  deny_writes ///< Require an OS-enforced sharing restriction; unsupported platforms fail closed.
};
/** Snapshot creation and verification outcome. */
enum class SnapshotStatus {
  /** Snapshot or source verification succeeded. */ ok,
  /** Stop was observed before completion. */ cancelled,
  /** Source identity, attributes, or contents changed. */ stale_input,
  /** The platform or source cannot supply required evidence. */ unverifiable,
  /** I/O or cryptographic operation failed. */ error
};
/** A bounded snapshot read result; zero bytes indicate EOF or failure. */
struct SnapshotRead { /** Status of the read. */ SnapshotStatus status{SnapshotStatus::error}; /** Bytes copied, or zero on failure. */ std::size_t bytes{}; };
/** Private immutable byte snapshot and retained original source, confined to one worker. Provisional platform implementation; no product integration. Linux requires O_TMPFILE and /proc self-descriptor access and fails closed without them. Other platforms retain their documented limitations. Not a complete product integration or an installation API. */
class FileSnapshot {
public:
  /** Release snapshot and original descriptors. A borrowing decoder must already be destroyed. */
  ~FileSnapshot();
  FileSnapshot(const FileSnapshot&) = delete;
  FileSnapshot& operator=(const FileSnapshot&) = delete;
  /** POSIX read-only descriptor borrowed until destruction; never close it. Returns -1 on Windows; use read_at there. */
  int borrowed_descriptor() const noexcept;
  /** Read at a byte offset on the owning worker. No buffer is retained; cancellation is checked before blocking I/O. */
  SnapshotRead read_at(std::uint64_t offset, std::span<std::byte> buffer, std::stop_token stop = {}) const;
  /** Fixed length of the finalized snapshot in bytes. */
  std::uint64_t size() const noexcept;
  /** Retained original modification time as signed Unix-epoch nanoseconds, or absent if not representable. This is evidence, not immutable identity. */
  std::optional<std::int64_t> modified_unix_ns() const noexcept;
  /** Digest of finalized snapshot bytes, prefixed with sha256:. */
  const std::string& digest() const noexcept;
  /** Rehash the finalized snapshot and retained original and recheck original/path identity; the decoder must have stopped using the snapshot. Cancellation is checked between reads; the optional byte-progress callback runs synchronously on the owning worker. A write or replacement after this check remains possible; unsupported identity evidence fails closed. */
  SnapshotStatus verify(std::stop_token stop = {}, const std::function<void(std::uintmax_t)>& progress = {}) const;
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
  /** Create a private snapshot of the source using bounded memory. No original file is written. I/O and the optional byte-progress observer run synchronously on the calling worker and cannot interrupt an in-flight system call. The optional deny-writes policy currently requires a resolved fixed-drive NTFS path and Windows share-mode enforcement, and fails closed elsewhere; it does not prove path or same-user safety. */
  static SnapshotOpen create(const std::filesystem::path& source, std::stop_token stop = {},
                             const std::function<void(std::uintmax_t)>& progress = {},
                             SnapshotSourcePolicy policy = SnapshotSourcePolicy::detect_changes);
};
}
