#include "beatglass/capture/file_reader.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {
void put16(std::ofstream& out, unsigned value) { out.put(static_cast<char>(value)); out.put(static_cast<char>(value >> 8)); }
void put32(std::ofstream& out, unsigned value) { put16(out, value & 0xffffu); put16(out, value >> 16); }
void write_wav(const std::filesystem::path& path) {
  std::ofstream out(path, std::ios::binary);
  out.write("RIFF", 4); put32(out, 44 - 8 + 8); out.write("WAVEfmt ", 8);
  put32(out, 16); put16(out, 1); put16(out, 1); put32(out, 8000); put32(out, 16000);
  put16(out, 2); put16(out, 16); out.write("data", 4); put32(out, 8);
  for (int i = 0; i < 4; ++i) put16(out, 0);
}
}

int main() {
  using namespace beatglass;
  bool invalid_epoch = false;
  try { FileReader reader("missing.wav", 0); }
  catch (const std::invalid_argument&) { invalid_epoch = true; }
  assert(invalid_epoch);

  const auto wav = std::filesystem::temp_directory_path() / "beatglass-reader.wav";
  std::error_code cleanup;
  write_wav(wav);
  {
    FileReader reader(wav, 7);
    assert(reader.metadata().sample_rate == 8000 && reader.metadata().channels == 1);
    assert(reader.metadata().frames == 4 && !reader.source_changed());
    bool empty_buffer_rejected = false;
    try { (void)reader.read(std::span<float>{}); }
    catch (const std::invalid_argument&) { empty_buffer_rejected = true; }
    assert(empty_buffer_rejected);
    std::vector<float> partial_samples(2);
    const auto partial_first = reader.read(partial_samples);
    assert(partial_first.status == FileReadStatus::data && partial_first.block.samples.size() == 2 && partial_first.block.first_frame == 0);
    const auto partial_second = reader.read(partial_samples);
    assert(partial_second.status == FileReadStatus::data && partial_second.block.samples.size() == 2 && partial_second.block.first_frame == 2);
    std::vector<float> samples(4);
    const auto first = reader.read(samples);
    assert(first.status == FileReadStatus::data && first.block.start_frame == 0 && first.block.epoch == 7);
    reader.seek(2, 9);
    const auto seeked = reader.read(samples);
    assert(seeked.status == FileReadStatus::data && seeked.block.start_frame == 2 && seeked.block.epoch == 9);
    bool stale_epoch = false;
    try { reader.seek(0, 9); } catch (const std::invalid_argument&) { stale_epoch = true; }
    assert(stale_epoch);
    reader.seek(4, 11);
    assert(reader.read(samples).status == FileReadStatus::end);
    bool out_of_range = false;
    try { reader.seek(5, 10); } catch (const std::out_of_range&) { out_of_range = true; }
    assert(out_of_range);
    const auto end = reader.read(samples);
    assert(end.status == FileReadStatus::end);
    const auto repeated_end = reader.read(samples);
    assert(repeated_end.status == FileReadStatus::end && repeated_end.block.samples.empty());
    reader.seek(0, 12);
    const auto replayed = reader.read(samples);
    assert(replayed.status == FileReadStatus::data && replayed.block.first_frame == 0 && replayed.block.epoch == 12);
    bool reused_epoch_rejected = false;
    try { reader.seek(1, 12); } catch (const std::invalid_argument&) { reused_epoch_rejected = true; }
    assert(reused_epoch_rejected);
    reader.seek(1, 13);
    const auto reseeked = reader.read(samples);
    assert(reseeked.status == FileReadStatus::data && reseeked.block.first_frame == 1 && reseeked.block.epoch == 13);
  }
  { std::fstream mutate(wav, std::ios::in | std::ios::out | std::ios::binary); mutate.seekp(44); mutate.put('\1'); }
  { FileReader reader(wav, 8); assert(reader.source_changed()); }
  const auto moved = wav.string() + ".moved";
  std::filesystem::rename(wav, moved);
  bool rename_failed = false;
  try { FileReader reader(wav, 10); }
  catch (const FileOpenError& error) { rename_failed = error.kind() == FileOpenErrorKind::unreadable; }
  assert(rename_failed);
  std::filesystem::remove(moved, cleanup);
  bool reopen_failed = false;
  try { FileReader reader(wav, 10); }
  catch (const FileOpenError& error) { reopen_failed = error.kind() == FileOpenErrorKind::unreadable; }
  assert(reopen_failed);

  const auto unsupported = std::filesystem::temp_directory_path() / "beatglass-reader.m4a";
  { std::ofstream out(unsupported, std::ios::binary); out << "not-audio"; }
  bool classified = false;
  try { FileReader reader(unsupported, 1); }
  catch (const FileOpenError& error) {
    classified = error.kind() == FileOpenErrorKind::unsupported_format;
  }
  assert(classified);
  const auto unsupported_upper = std::filesystem::temp_directory_path() / "beatglass-reader.M4A";
  { std::ofstream out(unsupported_upper, std::ios::binary); out << "not-audio"; }
  bool upper_classified = false;
  try { FileReader reader(unsupported_upper, 1); }
  catch (const FileOpenError& error) { upper_classified = error.kind() == FileOpenErrorKind::unsupported_format; }
  assert(upper_classified);
  const auto unsupported_aac = std::filesystem::temp_directory_path() / "beatglass-reader.aac";
  { std::ofstream out(unsupported_aac, std::ios::binary); out << "not-audio"; }
  bool aac_classified = false;
  try { FileReader reader(unsupported_aac, 1); }
  catch (const FileOpenError& error) { aac_classified = error.kind() == FileOpenErrorKind::unsupported_format; }
  assert(aac_classified);
  std::filesystem::remove(unsupported, cleanup);
  std::filesystem::remove(unsupported_upper, cleanup);
  std::filesystem::remove(unsupported_aac, cleanup);
  const auto empty_wav = std::filesystem::temp_directory_path() / "beatglass-reader-empty.wav";
  { std::ofstream empty(empty_wav, std::ios::binary); }
  bool empty_rejected = false;
  try { FileReader reader(empty_wav, 2); }
  catch (const FileOpenError& error) {
    empty_rejected = error.kind() == FileOpenErrorKind::unreadable || error.kind() == FileOpenErrorKind::invalid_format;
  }
  assert(empty_rejected);
  std::filesystem::remove(empty_wav, cleanup);
  bool directory_rejected = false;
  try { FileReader reader(std::filesystem::temp_directory_path(), 3); }
  catch (const FileOpenError& error) { directory_rejected = error.kind() == FileOpenErrorKind::unreadable; }
  assert(directory_rejected);
}
