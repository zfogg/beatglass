#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <stop_token>

namespace beatglass {
/** Byte-read outcome for a worker-owned source. */
enum class ByteReadStatus { data, end, cancelled, error };
/** Read outcome and byte count; only data may have a nonzero count. */
struct ByteReadResult {
  ByteReadStatus status{ByteReadStatus::error}; ///< Result category.
  std::size_t bytes{}; ///< Bytes written to the caller's buffer.
};
/** Base for a signed relative seek. */
enum class ByteSeekOrigin { begin, current, end };
/** Provisional seekable read-only byte source, confined to one worker.
 * A job may lend the same source to fingerprinting and a decoder sequentially.
 * The owner keeps the source alive until all borrowers finish. Reads may block;
 * no callback, GUI, or concurrent access is permitted. A source used for a
 * cacheable analysis must denote the same immutable snapshot throughout both
 * operations; this interface does not create or secure that snapshot. An opened
 * mutable file and its path are not by themselves immutable evidence.
 */
class ByteSource {
public:
  virtual ~ByteSource() = default;
  /** Size of the fixed byte sequence; no I/O is implied. */
  virtual std::uint64_t size() const = 0;
  /** Read from an absolute offset into nonempty caller-owned storage.
   * Cancellation already requested takes precedence over EOF and returns
   * cancelled. Otherwise, at or after size return end; a data result contains
   * 1..buffer.size() bytes and cannot extend beyond size. Interruption of a
   * blocking operation is implementation-dependent. A cancelled/error read
   * has zero bytes, but may have modified the output; discard its contents.
   * No buffer is retained after return. Empty buffers are not passed by ByteCursor.
   */
  virtual ByteReadResult read_at(std::uint64_t offset, std::span<std::byte> buffer,
                                 std::stop_token cancel) = 0;
};
/** Worker-local cursor borrowing a source; it does not extend source lifetime.
 * It validates source responses and advances only after valid data. A source
 * size change is a terminal error for this cursor; callers must reopen against
 * a fresh source snapshot. It is not thread-safe and cannot turn a mutable
 * underlying source into a snapshot.
 */
class ByteCursor {
public:
  /** Borrow a source and begin at byte zero. */
  explicit ByteCursor(ByteSource& source) : source_(source), snapshot_size_(source.size()) {}
  /** Current byte offset. */
  std::uint64_t position() const noexcept { return position_; }
  /** Seek to an absolute byte offset through EOF; reject offsets beyond EOF. */
  bool seek(std::uint64_t offset) {
    if (failed_) return false;
    if (offset > snapshot_size_) return false;
    position_ = offset; return true;
  }
  /** Seek by a signed displacement from begin, current position or EOF.
   * Reject underflow, overflow and positions past EOF without changing position.
   */
  bool seek_relative(std::int64_t displacement, ByteSeekOrigin origin) {
    if (failed_ || source_.size() != snapshot_size_) { failed_ = true; return false; }
    const auto size = snapshot_size_;
    std::uint64_t base;
    switch (origin) {
      case ByteSeekOrigin::begin: base = 0; break;
      case ByteSeekOrigin::current: base = position_; break;
      case ByteSeekOrigin::end: base = size; break;
      default: return false;
    }
    if (base > size) return false;
    if (displacement < 0) {
      const auto magnitude = static_cast<std::uint64_t>(-(displacement + 1)) + 1;
      if (magnitude > base) return false;
      position_ = base - magnitude;
    } else {
      const auto magnitude = static_cast<std::uint64_t>(displacement);
      if (magnitude > size - base) return false;
      position_ = base + magnitude;
    }
    return true;
  }
  /** Read nonempty output, retaining position on cancellation or error.
   * Cancellation already requested takes precedence over empty output and EOF;
   * otherwise empty output is an error, including at EOF. On cancellation or
   * error, the source may have modified output and the caller must discard it.
   * Reject malformed source responses; successful EOF does not advance.
   * Exceptions from the source propagate and leave position unchanged.
   */
  ByteReadResult read(std::span<std::byte> buffer, std::stop_token cancel = {}) {
    if (cancel.stop_requested()) return {ByteReadStatus::cancelled, 0};
    if (buffer.empty()) return {ByteReadStatus::error, 0};
    if (failed_ || source_.size() != snapshot_size_) { failed_ = true; return {ByteReadStatus::error, 0}; }
    const auto size = snapshot_size_;
    if (position_ > size) return {ByteReadStatus::error, 0};
    if (position_ == size) return {ByteReadStatus::end, 0};
    auto result = source_.read_at(position_, buffer, cancel);
    // A source may finish a non-interruptible read after cancellation was
    // requested. Cancellation wins over data or EOF, and the cursor must not
    // advance because the caller will discard the buffer.
    if (cancel.stop_requested()) return {ByteReadStatus::cancelled, 0};
    if (result.status == ByteReadStatus::data && result.bytes > 0 &&
        result.bytes <= buffer.size() && result.bytes <= size - position_) {
      position_ += result.bytes; return result;
    }
    if ((result.status == ByteReadStatus::cancelled || result.status == ByteReadStatus::error) && result.bytes == 0) return result;
    return {ByteReadStatus::error, 0};
  }
private:
  ByteSource& source_;
  const std::uint64_t snapshot_size_;
  std::uint64_t position_{};
  bool failed_{};
};
}
