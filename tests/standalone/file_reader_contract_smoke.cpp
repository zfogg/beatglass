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
  write_wav(wav);
  {
    FileReader reader(wav, 7);
    assert(reader.metadata().sample_rate == 8000 && reader.metadata().channels == 1);
    assert(reader.metadata().frames == 4 && !reader.source_changed());
    std::vector<float> samples(4);
    const auto first = reader.read(samples);
    assert(first.status == FileReadStatus::data && first.block.start_frame == 0 && first.block.epoch == 7);
    reader.seek(2, 9);
    const auto seeked = reader.read(samples);
    assert(seeked.status == FileReadStatus::data && seeked.block.start_frame == 2 && seeked.block.epoch == 9);
    bool stale_epoch = false;
    try { reader.seek(0, 9); } catch (const std::invalid_argument&) { stale_epoch = true; }
    assert(stale_epoch);
    const auto end = reader.read(samples);
    assert(end.status == FileReadStatus::end);
  }
  { std::ofstream append(wav, std::ios::app | std::ios::binary); append.put('\0'); }
  { FileReader reader(wav, 8); assert(reader.source_changed()); }
  std::error_code cleanup;
  std::filesystem::remove(wav, cleanup);

  const auto unsupported = std::filesystem::temp_directory_path() / "beatglass-reader.m4a";
  bool classified = false;
  try { FileReader reader(unsupported, 1); }
  catch (const FileOpenError& error) {
    classified = error.kind() == FileOpenErrorKind::unsupported_format;
  }
  assert(classified);
}
