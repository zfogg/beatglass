#include "virtual_io.hpp"
#include <sndfile.h>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace beatglass::detail {
VirtualIo::VirtualIo(ByteSource& source) : source_(source), cursor_(source) {}
void VirtualIo::set_cancel(std::stop_token cancel) noexcept { cancel_ = cancel; }
VirtualIoStatus VirtualIo::status() const noexcept { return status_; }
std::int64_t VirtualIo::reject_write() noexcept { if (status_ == VirtualIoStatus::ok) status_ = VirtualIoStatus::error; return 0; }
std::int64_t VirtualIo::length() noexcept {
  if (status_ != VirtualIoStatus::ok) return -1;
  try {
    const auto n = source_.size();
    if (n > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) { status_ = VirtualIoStatus::error; return -1; }
    return static_cast<std::int64_t>(n);
  } catch (...) { status_ = VirtualIoStatus::error; return -1; }
}
std::int64_t VirtualIo::tell() noexcept {
  if (status_ != VirtualIoStatus::ok) return -1;
  if (cursor_.position() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) { status_ = VirtualIoStatus::error; return -1; }
  return static_cast<std::int64_t>(cursor_.position());
}
std::int64_t VirtualIo::seek(std::int64_t offset, int origin) noexcept {
  if (status_ != VirtualIoStatus::ok) return -1;
  try {
    std::int64_t base;
    if (origin == SEEK_SET) base = 0;
    else if (origin == SEEK_CUR) base = tell();
    else if (origin == SEEK_END) base = length();
    else { status_ = VirtualIoStatus::error; return -1; }
    if (base < 0 || (offset > 0 && base > std::numeric_limits<std::int64_t>::max() - offset) ||
        (offset < 0 && offset < -base)) { status_ = VirtualIoStatus::error; return -1; }
    const auto target = base + offset;
    if (!cursor_.seek(static_cast<std::uint64_t>(target))) { status_ = VirtualIoStatus::error; return -1; }
    return target;
  } catch (...) { status_ = VirtualIoStatus::error; return -1; }
}
std::int64_t VirtualIo::read(void* output, std::int64_t count) noexcept {
  if (status_ != VirtualIoStatus::ok) return 0;
  if (count < 0 || (!output && count != 0) || static_cast<std::uint64_t>(count) > std::numeric_limits<std::size_t>::max()) { status_ = VirtualIoStatus::error; return 0; }
  if (count == 0) return 0;
  std::size_t done = 0; const auto size = static_cast<std::size_t>(count);
  try {
    while (done < size) {
      auto result = cursor_.read(std::span(static_cast<std::byte*>(output) + done, size - done), cancel_);
      if (result.status == ByteReadStatus::data) done += result.bytes;
      else if (result.status == ByteReadStatus::end) break;
      else { status_ = result.status == ByteReadStatus::cancelled ? VirtualIoStatus::cancelled : VirtualIoStatus::error; break; }
    }
  } catch (...) { status_ = VirtualIoStatus::error; }
  return static_cast<std::int64_t>(done);
}
namespace {
sf_count_t filelen(void* p) noexcept { return static_cast<VirtualIo*>(p)->length(); }
sf_count_t seek_cb(sf_count_t o, int w, void* p) noexcept { return static_cast<VirtualIo*>(p)->seek(o, w); }
sf_count_t read_cb(void* b, sf_count_t n, void* p) noexcept { return static_cast<VirtualIo*>(p)->read(b, n); }
sf_count_t write_cb(const void*, sf_count_t, void* p) noexcept { return static_cast<VirtualIo*>(p)->reject_write(); }
sf_count_t tell_cb(void* p) noexcept { return static_cast<VirtualIo*>(p)->tell(); }
SF_VIRTUAL_IO callbacks{filelen, seek_cb, read_cb, write_cb, tell_cb};
}
struct VirtualFileReader::Impl {
  VirtualIo io; SNDFILE* handle{}; SF_INFO info{}; FileMetadata meta; std::uint64_t epoch{}, position{}; bool ended{}, failed{};
  Impl(ByteSource& source, std::uint64_t e, std::stop_token token) : io(source), epoch(e) {
    if (token.stop_requested()) throw VirtualOpenCancelled();
    io.set_cancel(token); handle = sf_open_virtual(&callbacks, SFM_READ, &info, &io);
    if (!handle || io.status() != VirtualIoStatus::ok) {
      if (handle) sf_close(handle);
      if (io.status() == VirtualIoStatus::cancelled) throw VirtualOpenCancelled();
      throw std::runtime_error("Virtual audio open failed");
    }
    if (info.samplerate <= 0 || info.channels <= 0 || info.channels > std::numeric_limits<std::uint16_t>::max() || info.frames < 0) {
      sf_close(handle); handle = nullptr; throw std::runtime_error("Invalid audio format");
    }
    try { meta = {static_cast<std::uint32_t>(info.samplerate), static_cast<std::uint32_t>(info.channels), static_cast<std::uint64_t>(info.frames), (info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ? "WAV" : "audio"}; }
    catch (...) { sf_close(handle); handle = nullptr; throw; }
  }
  ~Impl() { if (handle) sf_close(handle); }
};
VirtualFileReader::VirtualFileReader(ByteSource& source, std::uint64_t epoch, std::stop_token cancel) : impl_(std::make_unique<Impl>(source, epoch, cancel)) {}
VirtualFileReader::~VirtualFileReader() = default;
const FileMetadata& VirtualFileReader::metadata() const noexcept { return impl_->meta; }
FileReadResult VirtualFileReader::read(std::span<float> buffer, std::stop_token cancel) {
  auto& f = *impl_; if (f.failed) throw std::runtime_error("Virtual reader failed");
  if (cancel.stop_requested()) return {FileReadStatus::cancelled, {}};
  if (buffer.empty() || buffer.size() % f.meta.channels || buffer.size() / f.meta.channels > static_cast<std::uint64_t>(std::numeric_limits<sf_count_t>::max())) throw std::invalid_argument("Buffer must hold whole frames");
  if (f.ended) return {FileReadStatus::end, {}};
  f.io.set_cancel(cancel); const auto n = sf_readf_float(f.handle, buffer.data(), static_cast<sf_count_t>(buffer.size() / f.meta.channels));
  if (f.io.status() != VirtualIoStatus::ok) { f.failed = true; if (f.io.status() == VirtualIoStatus::cancelled) return {FileReadStatus::cancelled, {}}; throw std::runtime_error("Virtual audio read failed"); }
  if (n <= 0) { if (sf_error(f.handle) != SF_ERR_NO_ERROR) { f.failed = true; throw std::runtime_error("Audio read failed"); } f.ended = true; return {FileReadStatus::end, {}}; }
  if (static_cast<std::uint64_t>(n) > std::numeric_limits<std::uint64_t>::max() - f.position) { f.failed = true; throw std::runtime_error("Frame position overflow"); }
  const auto start = f.position; f.position += static_cast<std::uint64_t>(n);
  return {FileReadStatus::data, {{f.meta.sample_rate, static_cast<std::uint16_t>(f.meta.channels)}, f.epoch, start, 0, false, buffer.first(static_cast<std::size_t>(n) * f.meta.channels)}};
}
void VirtualFileReader::seek(std::uint64_t frame) {
  auto& f = *impl_; if (f.failed) throw std::runtime_error("Virtual reader failed");
  if (frame > f.meta.frames) throw std::out_of_range("Seek outside file");
  if (f.epoch == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Epoch exhausted");
  f.io.set_cancel({}); const auto actual = sf_seek(f.handle, static_cast<sf_count_t>(frame), SEEK_SET);
  if (f.io.status() != VirtualIoStatus::ok || actual < 0 || static_cast<std::uint64_t>(actual) != frame) { f.failed = true; throw std::runtime_error("Virtual audio seek failed"); }
  ++f.epoch; f.position = frame; f.ended = false;
}
}
