#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/file_analysis.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <stop_token>
#include <stdexcept>
namespace beatglass {
class ByteSource;
/** Opening a borrowed source was cancelled. */
class FileOpenCancelled : public std::runtime_error {
public:
  /** Construct a typed cancellation exception. */
  FileOpenCancelled() : std::runtime_error("File open cancelled") {}
};
/** Classification for a local file that could not be opened by the decoder. */
enum class FileOpenErrorKind { unreadable, unsupported_format, invalid_format };
/** Typed decoder-open failure; no input file is modified by a failed open. */
class FileOpenError : public std::runtime_error {
public:
  /** Construct an open failure with its stable caller-facing category. */
  FileOpenError(FileOpenErrorKind kind, const std::string& message)
      : std::runtime_error(message), kind_(kind) {}
  /** Return whether the failure was an unsupported container/codec. */
  FileOpenErrorKind kind() const noexcept { return kind_; }
private:
  FileOpenErrorKind kind_;
};
/** Result category for a pull-based file read. */
enum class FileReadStatus {
  /** A nonempty block was decoded. */
  data,
  /** The decoder reached end of file. */
  end,
  /** Cancellation was observed before or during a borrowed-source read. */
  cancelled
};
/** One read result. The block view borrows the caller's output buffer. */
struct FileReadResult {
  /** Result category; block is populated only for data. */
  FileReadStatus status{FileReadStatus::end};
  /** Interleaved float PCM, valid until the caller reuses its output buffer. */
  AudioBlock block{};
};
/**
 * Worker-owned, pull-based decoded file source; not thread-safe.
 *
 * This opens a local file read-only and does not write tags or audio. It does
 * not pace reads or invoke hardware callbacks. All operations may block on
 * decoder/filesystem I/O and must stay off GUI and capture threads. The caller
 * provides an initial epoch unique within its session. A successful seek starts
 * a new epoch; existing borrowed blocks must be discarded by the caller.
 * Borrowed byte sources must remain alive through reader destruction and be
 * confined to the same worker. Snapshot immutability is the caller's duty.
 */
class FileReader {
public:
  /** Open a local file for reading.
   * @param path File path; the file is not modified.
   * @param initial_epoch Caller-owned unique epoch for this session.
   * @throws FileOpenError for an unreadable, unsupported, or invalid format.
   */
  explicit FileReader(const std::filesystem::path& path, std::uint64_t initial_epoch = 0);
  /** Open a borrowed, seekable byte source synchronously on the worker.
   * @param source Source that outlives this reader, including its destructor.
   * @param initial_epoch Nonzero process-coordinator-issued epoch for this source.
   * @param cancel Checked before and during reads; blocking I/O may not stop promptly.
   * @throws std::invalid_argument for zero epoch before accessing the source;
   * FileOpenCancelled if cancellation is observed while opening;
   * std::runtime_error on source or decoder errors.
   * The source must be an immutable snapshot for cacheable analysis. No source
   * is created, copied, verified or retained beyond the borrow by this class.
   */
  explicit FileReader(ByteSource& source, std::uint64_t initial_epoch, std::stop_token cancel = {});
  /** Close the decoder. The destructor may block on filesystem work. */
  ~FileReader();
  /** File readers cannot be copied. */
  FileReader(const FileReader&) = delete;
  /** File readers cannot be copy-assigned. */
  FileReader& operator=(const FileReader&) = delete;
  /** Read-only metadata, borrowed until this reader is destroyed. */
  const FileMetadata& metadata() const noexcept;
  /** Return whether a path-backed source's size or modification time changed
   * since open. This is a cheap guard, not a content-identity proof; borrowed
   * sources always return false.
   */
  bool source_changed() const noexcept;
  /** Pull normalized float frames into caller-owned interleaved storage.
   * @param buffer Nonempty storage sized to a whole number of source frames.
   * @param cancel Checked before I/O and during borrowed-source reads; blocking
   *               I/O may not be interruptible. An already-requested token
   *               leaves position unchanged. Mid-I/O cancellation invalidates
   *               the borrowed-source reader and may modify the output buffer.
   * @return Data, end, or cancelled. File replay has no live monotonic anchor;
   *         its block's monotonic_ns is zero and must not be used as one.
   * @throws std::invalid_argument on malformed buffer; std::runtime_error on
   *         decoder failure, after which this reader must be discarded.
   */
  FileReadResult read(std::span<float> buffer, std::stop_token cancel = {});
  /** Legacy path-only seek increments a local epoch; not process-unique.
   * Coordinated callers must use the explicit-epoch overload instead.
   * Seek to a frame and begin a fresh epoch, including when seeking to EOF.
   * @param frame Position from zero through the reported frame count.
   * @throws std::out_of_range for an invalid position; std::overflow_error if
   *         the epoch cannot advance; std::logic_error for borrowed sources;
   *         std::runtime_error on decoder failure,
   *         after which this reader must be discarded.
   */
  void seek(std::uint64_t frame);
  /** Seek with an externally allocated, strictly newer epoch.
   * @param frame Position from zero through the reported frame count.
   * @param new_epoch Coordinator-issued epoch greater than the current epoch.
   * @throws std::out_of_range for an invalid frame; std::invalid_argument for a
   * reused or decreasing epoch, both before decoder I/O; std::runtime_error
   * on decoder failure, after which this reader must be discarded. Epoch
   * allocation and process-wide uniqueness remain the caller's responsibility.
   */
  void seek(std::uint64_t frame, std::uint64_t new_epoch);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
