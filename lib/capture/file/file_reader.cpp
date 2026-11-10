#include "beatglass/capture/file_reader.hpp"
#include "virtual_io.hpp"
#include <sndfile.h>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <system_error>
namespace beatglass {
namespace {
bool unsupported_extension(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext == ".aac" || ext == ".m4a";
}
bool known_audio_extension(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext == ".wav" || ext == ".flac" || ext == ".mp3" || ext == ".aif" || ext == ".aiff";
}
}
struct FileReader::Impl {
  SNDFILE* handle{}; SF_INFO info{}; FileMetadata metadata; std::filesystem::path path; std::filesystem::file_time_type initial_modified{}; std::uintmax_t initial_size{}; std::uint64_t epoch{}, position{}; bool ended{}, failed{};
  std::unique_ptr<detail::VirtualFileReader> virtual_reader;
  bool source_changed() const noexcept {
    if (path.empty()) return false;
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size != initial_size) return true;
    const auto modified = std::filesystem::last_write_time(path, ec);
    return ec || modified != initial_modified;
  }
  Impl(const std::filesystem::path& path, std::uint64_t initial) : path(path), epoch(initial) {
#ifdef _WIN32
    handle = sf_wchar_open(path.c_str(), SFM_READ, &info);
#else
    handle = sf_open(path.c_str(), SFM_READ, &info);
#endif
    if (!handle) {
      if (unsupported_extension(path))
        throw FileOpenError(FileOpenErrorKind::unsupported_format,
                            "Unsupported audio format (AAC/M4A is not enabled)");
      std::error_code ec;
      if (known_audio_extension(path) && std::filesystem::is_regular_file(path, ec) && !ec)
        throw FileOpenError(FileOpenErrorKind::invalid_format, "Invalid audio format");
      throw FileOpenError(FileOpenErrorKind::unreadable, "Could not open audio file");
    }
    if (info.samplerate <= 0 || info.channels <= 0 || info.channels > std::numeric_limits<std::uint16_t>::max() || info.frames < 0) {
      sf_close(handle); handle = nullptr;
      throw FileOpenError(FileOpenErrorKind::invalid_format, "Invalid audio format");
    }
    try {
      metadata = {static_cast<std::uint32_t>(info.samplerate), static_cast<std::uint32_t>(info.channels),
        static_cast<std::uint64_t>(info.frames), (info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ? "WAV" : "audio"};
      std::error_code ec; initial_size = std::filesystem::file_size(path, ec); if (ec) throw std::system_error(ec, "file_size");
      initial_modified = std::filesystem::last_write_time(path, ec); if (ec) throw std::system_error(ec, "last_write_time");
    } catch (...) { sf_close(handle); handle = nullptr; throw; }
  }
  Impl(ByteSource& source, std::uint64_t initial, std::stop_token cancel) : epoch(initial),
      virtual_reader(std::make_unique<detail::VirtualFileReader>(source, initial, cancel)) {
    metadata = virtual_reader->metadata();
  }
  ~Impl() { if (handle) sf_close(handle); }
  bool source_changed() const noexcept {
    if (path.empty()) return false;
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size != initial_size) return true;
    const auto modified = std::filesystem::last_write_time(path, ec);
    return ec || modified != initial_modified;
  }
};
FileReader::FileReader(const std::filesystem::path& path, std::uint64_t epoch) {
  if (!epoch) throw std::invalid_argument("File source epoch must be nonzero");
  impl_ = std::make_unique<Impl>(path, epoch);
}
FileReader::FileReader(ByteSource& source, std::uint64_t epoch, std::stop_token cancel) {
  if (!epoch) throw std::invalid_argument("Borrowed source epoch must be nonzero");
  impl_ = std::make_unique<Impl>(source, epoch, cancel);
}
FileReader::~FileReader() = default;
const FileMetadata& FileReader::metadata() const noexcept { return impl_->metadata; }
bool FileReader::source_changed() const noexcept { return impl_->source_changed(); }
FileReadResult FileReader::read(std::span<float> buffer, std::stop_token cancel) {
  auto& f = *impl_; if (f.failed) throw std::runtime_error("File reader failed");
  if (f.virtual_reader) return f.virtual_reader->read(buffer, cancel);
  if (cancel.stop_requested()) return {FileReadStatus::cancelled, {}};
  if (buffer.empty() || buffer.size() % f.metadata.channels || buffer.size() / f.metadata.channels > static_cast<std::uint64_t>(std::numeric_limits<sf_count_t>::max())) throw std::invalid_argument("Buffer must hold whole frames");
  if (f.source_changed()) { f.failed = true; throw std::runtime_error("Audio source changed during read"); }
  if (f.ended) return {FileReadStatus::end, {}};
  const auto frames = sf_readf_float(f.handle, buffer.data(), static_cast<sf_count_t>(buffer.size() / f.metadata.channels));
  if (frames <= 0) {
    if (sf_error(f.handle) != SF_ERR_NO_ERROR) { f.failed = true; throw std::runtime_error("Audio read failed"); }
    f.ended = true; return {FileReadStatus::end, {}};
  }
  if (static_cast<std::uint64_t>(frames) > std::numeric_limits<std::uint64_t>::max() - f.position) { f.failed = true; throw std::runtime_error("Frame position overflow"); }
  const auto start = f.position; f.position += static_cast<std::uint64_t>(frames);
  AudioBlock block{{f.metadata.sample_rate, static_cast<std::uint16_t>(f.metadata.channels)}, f.epoch, start, 0, false,
                   buffer.first(static_cast<std::size_t>(frames) * f.metadata.channels)};
  return {FileReadStatus::data, block};
}
void FileReader::seek(std::uint64_t frame) {
  auto& f = *impl_; if (f.failed) throw std::runtime_error("File reader failed");
  if (f.virtual_reader) throw std::logic_error("Borrowed sources require an explicit epoch");
  if (frame > f.metadata.frames) throw std::out_of_range("Seek outside file");
  if (f.epoch == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Epoch exhausted");
  if (f.source_changed()) { f.failed = true; throw std::runtime_error("Audio source changed during seek"); }
  seek(frame, f.epoch + 1);
}
void FileReader::seek(std::uint64_t frame, std::uint64_t new_epoch) {
  auto& f = *impl_; if (f.failed) throw std::runtime_error("File reader failed");
  if (frame > f.metadata.frames) throw std::out_of_range("Seek outside file");
  if (new_epoch <= f.epoch) throw std::invalid_argument("Epoch must increase");
  if (f.source_changed()) { f.failed = true; throw std::runtime_error("Audio source changed during seek"); }
  if (f.virtual_reader) { f.virtual_reader->seek(frame, new_epoch); f.epoch = new_epoch; return; }
  const auto actual = sf_seek(f.handle, static_cast<sf_count_t>(frame), SEEK_SET);
  if (actual < 0 || static_cast<std::uint64_t>(actual) != frame) { f.failed = true; throw std::runtime_error("Audio seek failed"); }
  f.epoch = new_epoch; f.position = frame; f.ended = false;
}
}
