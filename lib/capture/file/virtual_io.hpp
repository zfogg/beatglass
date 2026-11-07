#pragma once
#include "beatglass/capture/byte_source.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <stop_token>

namespace beatglass::detail {
/** Sticky virtual-I/O state; cancellation and errors invalidate the decoder. */
enum class VirtualIoStatus { ok, cancelled, error };
/** Synchronous worker-only virtual I/O over a borrowed fixed-size byte source.
 * The source must outlive this object and any decoder using it. Calls may block;
 * the caller serializes access and must not call from audio or GUI threads.
 * Exceptions are caught and become sticky errors. No bytes are retained.
 */
class VirtualIo {
public:
  /** Borrow the source; its immutable size must fit signed 64-bit offsets. */
  explicit VirtualIo(ByteSource& source);
  /** Set the token used by subsequent operations; does not clear sticky state. */
  void set_cancel(std::stop_token cancel) noexcept;
  /** Return byte length or -1 on failure, recording the failure. */
  std::int64_t length() noexcept;
  /** Apply SEEK_SET, SEEK_CUR or SEEK_END with checked signed arithmetic.
   * @return Absolute position, or -1 for invalid/out-of-range positions.
   */
  std::int64_t seek(std::int64_t offset, int origin) noexcept;
  /** Return current position, or -1 after a failure. */
  std::int64_t tell() noexcept;
  /** Fill up to count bytes, accumulating short reads; zero count is a no-op.
   * Return bytes copied (possibly partial) and retain any terminal failure in status().
   * If status is non-ok, discard the entire output: the source may have
   * modified bytes beyond the reported prefix before failing.
   */
  std::int64_t read(void* output, std::int64_t count) noexcept;
  /** Reject a write attempt and record a sticky error. */
  std::int64_t reject_write() noexcept;
  /** Return sticky status; it remains non-ok until destruction. */
  VirtualIoStatus status() const noexcept;
private:
  ByteSource& source_;
  ByteCursor cursor_;
  std::stop_token cancel_;
  VirtualIoStatus status_{VirtualIoStatus::ok};
};
/** Open was cancelled before or during virtual I/O. */
class VirtualOpenCancelled : public std::runtime_error {
public:
  /** Construct the cancellation exception. */
  VirtualOpenCancelled() : std::runtime_error("Virtual audio open cancelled") {}
};
/** Experimental worker-owned libsndfile decoder over borrowed virtual I/O.
 * The source outlives this reader, including its destructor. It must denote an
 * immutable snapshot for cacheable work; this class does not create one. No
 * product path uses this decoder yet. Cancellation during I/O invalidates it.
 */
class VirtualFileReader {
public:
  /** Open with a coordinator-allocated initial epoch; throws VirtualOpenCancelled
   * for cancellation and std::runtime_error for failure. */
  explicit VirtualFileReader(ByteSource& source, std::uint64_t epoch, std::stop_token cancel = {});
  /** Close the decoder before releasing the borrowed source. */
  ~VirtualFileReader();
  VirtualFileReader(const VirtualFileReader&) = delete;
  VirtualFileReader& operator=(const VirtualFileReader&) = delete;
  /** Metadata borrowed until destruction. */
  const FileMetadata& metadata() const noexcept;
  /** Decode into caller storage; mid-I/O cancellation returns cancelled and makes this reader unusable.
   * @throws std::exception on malformed buffers or I/O/decode errors. Borrowed output
   * is valid only until the caller reuses its buffer. A cancelled or failed read may
   * have modified the caller buffer, but publishes no samples.
   */
  FileReadResult read(std::span<float> buffer, std::stop_token cancel = {});
  /** Seek using an externally allocated, strictly increasing epoch.
   * Reject an invalid frame or reused/non-increasing epoch before decoder I/O;
   * a decoder failure invalidates the reader. The coordinator must guarantee
   * process-wide uniqueness, including across recreated readers.
   */
  void seek(std::uint64_t frame, std::uint64_t new_epoch);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
