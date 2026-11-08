#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iterator>
#include <vector>
#include <random>
#include <stop_token>
using namespace beatglass;
namespace {
struct WaveFixture {
  std::filesystem::path path;
  explicit WaveFixture(unsigned rate = 44100) {
    std::random_device random;
    path = std::filesystem::temp_directory_path() / ("beatglass-test-" + std::to_string(random()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
    if (std::filesystem::exists(path)) throw std::runtime_error("Fixture path already exists");
    std::ofstream out(path, std::ios::binary);
    auto text = [&](const char* bytes, int n) { out.write(bytes, n); };
    auto u16 = [&](unsigned n) { char b[2] = {char(n & 255), char((n >> 8) & 255)}; out.write(b, 2); };
    auto u32 = [&](unsigned n) { u16(n & 65535); u16(n >> 16); };
    text("RIFF",4); u32(52); text("WAVEfmt ",8); u32(16); u16(1); u16(2); u32(rate); u32(rate * 4); u16(4); u16(16); text("data",4); u32(16);
    for (unsigned n : {0U, 32767U, 8192U, 57344U, 16384U, 49152U, 24576U, 40960U}) u16(n);
    if (!out) throw std::runtime_error("Could not create fixture");
  }
  ~WaveFixture() { std::error_code ignored; std::filesystem::remove(path, ignored); }
};
}
TEST_CASE("file reader pulls frames and seek starts a new epoch") {
 WaveFixture fixture; FileReader reader(fixture.path, 42); const auto meta = reader.metadata();
 REQUIRE(meta.sample_rate == 44100); REQUIRE(meta.channels == 2); REQUIRE(meta.frames == 4);
 std::array<float,4> samples{}; auto first = reader.read(samples);
 REQUIRE(first.status == FileReadStatus::data); CHECK(first.block.epoch == 42); CHECK(first.block.first_frame == 0);
 REQUIRE(first.block.samples.size() == 4); CHECK(samples[0] == Catch::Approx(0)); CHECK(samples[1] == Catch::Approx(32767.0/32768));
 StreamContinuity continuity; REQUIRE(continuity.observe(first.block) == StreamTransition::acquire);
 auto next = reader.read(samples); REQUIRE(next.status == FileReadStatus::data); CHECK(next.block.first_frame == 2);
 CHECK(continuity.observe(next.block) == StreamTransition::contiguous);
 CHECK(reader.read(samples).status == FileReadStatus::end); CHECK(reader.read(samples).status == FileReadStatus::end);
 reader.seek(1); auto replay = reader.read(samples); REQUIRE(replay.status == FileReadStatus::data);
 CHECK(replay.block.epoch == 43); CHECK(replay.block.first_frame == 1); CHECK(samples[0] == Catch::Approx(.25));
 CHECK(continuity.observe(replay.block) == StreamTransition::reacquire);
 reader.seek(meta.frames); CHECK(reader.read(samples).status == FileReadStatus::end);
}
TEST_CASE("cancelled and invalid pulls leave position unchanged") {
 WaveFixture fixture; FileReader reader(fixture.path, 7); std::array<float,4> samples{9,9,9,9};
 std::stop_source stop; stop.request_stop(); CHECK(reader.read(samples, stop.get_token()).status == FileReadStatus::cancelled);
 CHECK(samples[0] == 9); std::array<float,3> bad{}; CHECK_THROWS_AS(reader.read(bad), std::invalid_argument);
 CHECK_THROWS_AS(reader.seek(5), std::out_of_range);
 auto first = reader.read(samples); REQUIRE(first.status == FileReadStatus::data); CHECK(first.block.first_frame == 0); CHECK(first.block.epoch == 7);
}
TEST_CASE("cancellation after playback progress preserves the next frame") {
 WaveFixture fixture; FileReader reader(fixture.path, 7); std::array<float,2> samples{};
 REQUIRE(reader.read(samples).status == FileReadStatus::data);
 std::stop_source stop; stop.request_stop();
 CHECK(reader.read(samples, stop.get_token()).status == FileReadStatus::cancelled);
 auto resumed = reader.read(samples); REQUIRE(resumed.status == FileReadStatus::data);
 CHECK(resumed.block.first_frame == 1); CHECK(resumed.block.epoch == 7);
}
TEST_CASE("seek refuses epoch overflow") {
 WaveFixture fixture; FileReader reader(fixture.path, std::numeric_limits<std::uint64_t>::max());
 CHECK_THROWS_AS(reader.seek(0), std::overflow_error);
 std::array<float,2> samples{}; auto first = reader.read(samples);
 REQUIRE(first.status == FileReadStatus::data); CHECK(first.block.first_frame == 0);
}

TEST_CASE("path reader rejects zero epoch before opening source") {
  WaveFixture fixture;
  CHECK_THROWS_AS(FileReader(fixture.path, 0), std::invalid_argument);
}

TEST_CASE("path reader classifies AAC and M4A as unsupported instead of unreadable") {
  for (const auto extension : {".aac", ".m4a", ".AAC", ".M4A"}) {
    const auto path = std::filesystem::temp_directory_path() /
      ("beatglass-unsupported" + std::string(extension));
    { std::ofstream out(path, std::ios::binary); out << "not a decoder fixture"; }
    try {
      try { FileReader reader(path, 1); FAIL("unsupported input opened"); }
      catch (const FileOpenError& error) {
        CHECK(error.kind() == FileOpenErrorKind::unsupported_format);
      }
    } catch (...) { std::filesystem::remove(path); throw; }
    std::filesystem::remove(path);
  }
}

TEST_CASE("file source changes sample rate only across distinct epochs") {
 WaveFixture first_file(44100), second_file(48000); std::array<float,4> first_buffer{}, second_buffer{};
 FileReader first(first_file.path, 1), second(second_file.path, 2); StreamContinuity continuity;
 const auto a = first.read(first_buffer); const auto b = second.read(second_buffer);
 REQUIRE(a.status == FileReadStatus::data); REQUIRE(b.status == FileReadStatus::data);
 CHECK(a.block.format.sample_rate == 44100); CHECK(b.block.format.sample_rate == 48000);
 CHECK(continuity.observe(a.block) == StreamTransition::acquire);
 CHECK(continuity.observe(b.block) == StreamTransition::reacquire);
}

TEST_CASE("reading and seeking leave the source file unchanged") {
 WaveFixture fixture;
 auto bytes = [&] { std::ifstream in(fixture.path, std::ios::binary); return std::vector<char>(std::istreambuf_iterator<char>(in), {}); };
 const auto before = bytes(); REQUIRE_FALSE(before.empty());
 { FileReader reader(fixture.path, 1); std::array<float,4> samples{}; REQUIRE(reader.read(samples).status == FileReadStatus::data); reader.seek(0); REQUIRE(reader.read(samples).status == FileReadStatus::data); }
 CHECK(bytes() == before);
}
TEST_CASE("path reader accepts externally allocated epochs without moving on rejection") {
 WaveFixture fixture; FileReader reader(fixture.path, 7); std::array<float,2> samples{};
 REQUIRE(reader.read(samples).status == FileReadStatus::data);
 CHECK_THROWS_AS(reader.seek(0,7),std::invalid_argument); CHECK_THROWS_AS(reader.seek(0,6),std::invalid_argument);
 const auto unchanged=reader.read(samples); REQUIRE(unchanged.status==FileReadStatus::data); CHECK(unchanged.block.first_frame==1); CHECK(unchanged.block.epoch==7);
 reader.seek(0,42); const auto changed=reader.read(samples); REQUIRE(changed.status==FileReadStatus::data); CHECK(changed.block.epoch==42); CHECK(changed.block.first_frame==0);
}
TEST_CASE("path reader reports source metadata changes") {
 WaveFixture fixture; FileReader reader(fixture.path, 11); CHECK_FALSE(reader.source_changed());
 { std::ofstream out(fixture.path, std::ios::binary | std::ios::app); out << 'x'; }
 CHECK(reader.source_changed());
}
