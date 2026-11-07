#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/file_analysis.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <stop_token>
namespace beatglass {
/** Result category for a pull-based file read. */
enum class FileReadStatus {
  /** A nonempty block was decoded. */
  data,
  /** The decoder reached end of file. */
  end,
  /** Cancellation was already requested before the read. */
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
 */
class FileReader {
public:
  /** Open a local file for reading.
   * @param path File path; the file is not modified.
   * @param initial_epoch Caller-owned unique epoch for this session.
   * @throws std::runtime_error for an unreadable file or invalid format.
   */
  explicit FileReader(const std::filesystem::path& path, std::uint64_t initial_epoch = 0);
  /** Close the decoder. The destructor may block on filesystem work. */
  ~FileReader();
  /** File readers cannot be copied. */
  FileReader(const FileReader&) = delete;
  /** File readers cannot be copy-assigned. */
  FileReader& operator=(const FileReader&) = delete;
  /** Read-only metadata, borrowed until this reader is destroyed. */
  const FileMetadata& metadata() const noexcept;
  /** Pull normalized float frames into caller-owned interleaved storage.
   * @param buffer Nonempty storage sized to a whole number of source frames.
   * @param cancel Checked before I/O; cancellation cannot interrupt an ongoing
   *               decoder read and does not change position when already set.
   * @return Data, end, or cancelled. File replay has no live monotonic anchor;
   *         its block's monotonic_ns is zero and must not be used as one.
   * @throws std::invalid_argument on malformed buffer; std::runtime_error on
   *         decoder failure, after which this reader must be discarded.
   */
  FileReadResult read(std::span<float> buffer, std::stop_token cancel = {});
  /** Seek to a frame and begin a fresh epoch, including when seeking to EOF.
   * @param frame Position from zero through the reported frame count.
   * @throws std::out_of_range for an invalid position; std::overflow_error if
   *         the epoch cannot advance; std::runtime_error on decoder failure,
   *         after which this reader must be discarded.
   */
  void seek(std::uint64_t frame);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
