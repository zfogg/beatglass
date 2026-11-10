#include "../../lib/capture/file/virtual_io.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
using namespace beatglass;
using namespace beatglass::detail;
namespace {
class MemorySource : public ByteSource {
public:
  std::vector<std::byte> bytes;
  std::size_t limit{3};
  int calls{}, fail_after{-1};
  bool throws{}, cancel_on_read{};
  int cancel_after{-1};
  std::stop_source* stop{};
  std::uint64_t size() const override { if (throws) throw std::runtime_error("size"); return bytes.size(); }
  ByteReadResult read_at(std::uint64_t pos, std::span<std::byte> out, std::stop_token token) override {
    ++calls;
    if (throws) throw std::runtime_error("read");
    if ((cancel_on_read || (cancel_after >= 0 && calls > cancel_after)) && stop) { if (!out.empty()) out[0]=std::byte{0x5a}; stop->request_stop(); return {ByteReadStatus::cancelled, 0}; }
    if (token.stop_requested()) return {ByteReadStatus::cancelled, 0};
    if (fail_after >= 0 && calls > fail_after) { if (!out.empty()) out[0]=std::byte{0x6b}; return {ByteReadStatus::error, 0}; }
    if (pos >= bytes.size()) return {ByteReadStatus::end, 0};
    auto n = std::min({limit, out.size(), bytes.size() - static_cast<std::size_t>(pos)});
    std::copy_n(bytes.begin() + static_cast<std::size_t>(pos), n, out.begin()); return {ByteReadStatus::data, n};
  }
};
MemorySource wav() {
  MemorySource source;
  auto text = [&](const char* p, int n) { for (int i=0; i<n; ++i) source.bytes.push_back(static_cast<std::byte>(p[i])); };
  auto u16 = [&](unsigned v) { source.bytes.push_back(static_cast<std::byte>(v & 255)); source.bytes.push_back(static_cast<std::byte>((v >> 8) & 255)); };
  auto u32 = [&](unsigned v) { u16(v & 65535); u16(v >> 16); };
  constexpr unsigned frames = 8192;
  text("RIFF",4); u32(36 + frames * 2); text("WAVEfmt ",8); u32(16); u16(1); u16(1); u32(44100); u32(88200); u16(2); u16(16); text("data",4); u32(frames * 2);
  for (unsigned i=0; i<frames; ++i) u16(i % 2 ? 8192 : 0);
  return source;
}
}
TEST_CASE("virtual I/O accumulates short reads and validates signed seeks") {
  auto src = wav(); VirtualIo io(src); std::array<std::byte,10> out{};
  CHECK(io.read(nullptr, 0) == 0); CHECK(io.status() == VirtualIoStatus::ok);
  CHECK(io.length() == static_cast<std::int64_t>(src.bytes.size()));
  CHECK(io.read(out.data(), out.size()) == 10); CHECK(src.calls >= 4); CHECK(io.tell() == 10);
  CHECK(io.seek(-5, SEEK_CUR) == 5); CHECK(io.seek(-2, SEEK_END) == static_cast<std::int64_t>(src.bytes.size()-2));
  CHECK(io.read(out.data(), out.size()) == 2); CHECK(io.read(out.data(), out.size()) == 0); CHECK(io.status() == VirtualIoStatus::ok);
  CHECK(io.seek(0, SEEK_SET) == 0);
  CHECK(io.seek(std::numeric_limits<std::int64_t>::min(), SEEK_CUR) == -1); CHECK(io.status() == VirtualIoStatus::error);
  VirtualIo overflow(src); CHECK(overflow.seek(std::numeric_limits<std::int64_t>::max(), SEEK_END) == -1); CHECK(overflow.status() == VirtualIoStatus::error);
  VirtualIo beyond(src); CHECK(beyond.seek(1, SEEK_END) == -1); CHECK(beyond.status() == VirtualIoStatus::error);
}
TEST_CASE("virtual I/O retains partial failures cancellation and exceptions") {
  auto src = wav(); std::array<std::byte,10> out{}; src.fail_after = 1; VirtualIo error(src);
  CHECK(error.read(out.data(), out.size()) == 3); CHECK(error.status() == VirtualIoStatus::error); CHECK(std::to_integer<int>(out[3]) == 0x6b); CHECK(error.read(out.data(), 1) == 0);
  auto throwing = wav(); VirtualIo bad(throwing); throwing.throws = true; CHECK(bad.read(out.data(), 1) == 0); CHECK(bad.status() == VirtualIoStatus::error);
  auto cancelled = wav(); std::stop_source stop; cancelled.stop = &stop; cancelled.cancel_on_read = true; VirtualIo io(cancelled); io.set_cancel(stop.get_token());
  CHECK(io.read(out.data(), 1) == 0); CHECK(io.status() == VirtualIoStatus::cancelled); CHECK(std::to_integer<int>(out[0]) == 0x5a);
  CHECK(io.tell() == -1); CHECK(io.status() == VirtualIoStatus::cancelled);
  auto partial = wav(); std::stop_source later; partial.stop = &later; partial.cancel_after = 1; VirtualIo partial_io(partial); partial_io.set_cancel(later.get_token());
  CHECK(partial_io.read(out.data(), 10) == 3); CHECK(partial_io.status() == VirtualIoStatus::cancelled); CHECK(std::to_integer<int>(out[3]) == 0x5a);
}
TEST_CASE("virtual libsndfile decoder reads and seeks the borrowed bytes") {
  auto src = wav(); VirtualFileReader decoder(src, 7); REQUIRE(decoder.metadata().frames == 8192); REQUIRE(decoder.metadata().channels == 1);
  std::array<float,4> samples{}; auto first = decoder.read(samples); REQUIRE(first.status == FileReadStatus::data);
  CHECK(first.block.epoch == 7); CHECK(first.block.first_frame == 0); CHECK(samples[1] == Catch::Approx(.25));
  CHECK_THROWS_AS(decoder.seek(2, 7), std::invalid_argument);
  CHECK_THROWS_AS(decoder.seek(2, 6), std::invalid_argument);
  const auto unchanged = decoder.read(samples); REQUIRE(unchanged.status == FileReadStatus::data);
  CHECK(unchanged.block.first_frame == 4); CHECK(unchanged.block.epoch == 7);
  decoder.seek(2, 42); auto after = decoder.read(samples); REQUIRE(after.status == FileReadStatus::data); CHECK(after.block.epoch == 42); CHECK(after.block.first_frame == 2);
  decoder.seek(decoder.metadata().frames, 100); CHECK(decoder.read(samples).status == FileReadStatus::end);
}
TEST_CASE("virtual decoder reports callback failures and cancellation") {
  auto src = wav(); src.limit = 1024; VirtualFileReader decoder(src, 1); std::array<float,1024> samples{};
  src.fail_after = src.calls; CHECK_THROWS(decoder.read(samples)); CHECK_THROWS(decoder.read(samples));
  auto other = wav(); other.limit = 1024; VirtualFileReader cancelled(other, 1); std::stop_source stop; other.stop = &stop; other.cancel_on_read = true;
  const auto cancelled_read = cancelled.read(samples, stop.get_token());
  CHECK(cancelled_read.status == FileReadStatus::cancelled); CHECK(cancelled_read.block.samples.empty()); CHECK_THROWS(cancelled.read(samples));
}
TEST_CASE("virtual decoder matches path decoder on generated v1 codec fixtures") {
  const auto root = std::filesystem::path(__FILE__).parent_path().parent_path() / "fixtures" / "generated";
#ifdef BEATGLASS_REQUIRE_V1_CODECS
  const std::vector<const char*> names{"tone.aiff", "tone.flac", "tone.mp3"};
#else
  const std::vector<const char*> names{"tone.aiff"};
#endif
  for (const auto* name : names) {
    const auto path = root / name;
    std::ifstream in(path, std::ios::binary); REQUIRE(in);
    std::vector<char> bytes(std::istreambuf_iterator<char>{in}, {}); REQUIRE_FALSE(bytes.empty());
    MemorySource source; source.limit = 37; source.bytes.reserve(bytes.size());
    for (char c : bytes) source.bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    FileReader path_reader(path, 1); VirtualFileReader virtual_reader(source, 1);
    CHECK(virtual_reader.metadata().sample_rate == path_reader.metadata().sample_rate);
    CHECK(virtual_reader.metadata().channels == path_reader.metadata().channels);
    CHECK(virtual_reader.metadata().frames == path_reader.metadata().frames);
    std::vector<float> a(virtual_reader.metadata().channels * 256), b(a.size());
    auto ar = path_reader.read(a), br = virtual_reader.read(b);
    REQUIRE(ar.status == FileReadStatus::data); REQUIRE(br.status == FileReadStatus::data);
    REQUIRE(ar.block.samples.size() == br.block.samples.size());
    for (std::size_t i=0; i<ar.block.samples.size(); ++i) CHECK(a[i] == Catch::Approx(b[i]));
  }
}
TEST_CASE("virtual decoder distinguishes open cancellation from failure") {
  auto src = wav(); std::stop_source stop; stop.request_stop();
  CHECK_THROWS_AS(VirtualFileReader(src, 0, stop.get_token()), FileOpenCancelled);
  auto during = wav(); std::stop_source mid; during.stop = &mid; during.cancel_on_read = true;
  CHECK_THROWS_AS(VirtualFileReader(during, 0, mid.get_token()), FileOpenCancelled);
  auto failing = wav(); failing.fail_after = 0;
  CHECK_THROWS_AS(VirtualFileReader(failing, 1), std::runtime_error);
}
TEST_CASE("virtual I/O rejects unrepresentable sizes and unsupported writes") {
  class HugeSource : public ByteSource {
  public:
    std::uint64_t size() const override { return std::numeric_limits<std::uint64_t>::max(); }
    ByteReadResult read_at(std::uint64_t, std::span<std::byte>, std::stop_token) override { return {ByteReadStatus::end, 0}; }
  } huge;
  VirtualIo io(huge); CHECK(io.length() == -1); CHECK(io.status() == VirtualIoStatus::error);
  auto source = wav(); VirtualIo writer(source); CHECK(writer.reject_write() == 0); CHECK(writer.status() == VirtualIoStatus::error);
  VirtualIo origin(source); CHECK(origin.seek(0, -1) == -1); CHECK(origin.status() == VirtualIoStatus::error);
}
TEST_CASE("virtual decoder accepts coordinator epochs without wrapping") {
  auto src = wav(); VirtualFileReader decoder(src, 1); std::array<float, 2> samples{};
  decoder.seek(10, std::numeric_limits<std::uint64_t>::max());
  CHECK_THROWS_AS(decoder.seek(0, std::numeric_limits<std::uint64_t>::max()), std::invalid_argument);
  const auto block = decoder.read(samples); REQUIRE(block.status == FileReadStatus::data);
  CHECK(block.block.epoch == std::numeric_limits<std::uint64_t>::max()); CHECK(block.block.first_frame == 10);
}
TEST_CASE("FileReader borrows byte source and requires externally allocated seek epochs") {
  auto source=wav(); const auto before=source.bytes;
  {
    FileReader reader(source, 10); REQUIRE(reader.metadata().frames==8192);
    std::array<float,4> output{}; auto first=reader.read(output); REQUIRE(first.status==FileReadStatus::data);
    CHECK(first.block.epoch==10); CHECK(first.block.first_frame==0); CHECK(output[1]==Catch::Approx(.25));
    CHECK_THROWS_AS(reader.seek(2),std::logic_error);
    CHECK_THROWS_AS(reader.seek(2,10),std::invalid_argument);
    auto next=reader.read(output); REQUIRE(next.status==FileReadStatus::data); CHECK(next.block.first_frame==4); CHECK(next.block.epoch==10);
    reader.seek(2,42); auto after=reader.read(output); REQUIRE(after.status==FileReadStatus::data); CHECK(after.block.epoch==42); CHECK(after.block.first_frame==2);
    reader.seek(reader.metadata().frames,100); CHECK(reader.read(output).status==FileReadStatus::end);
  }
  CHECK((source.bytes==before));
}

TEST_CASE("FileReader seek reacquisition publishes only the new epoch") {
  auto source=wav(); FileReader reader(source, 1); std::array<float,4> output{};
  reader.seek(0,11); auto first=reader.read(output);
  REQUIRE(first.status==FileReadStatus::data); CHECK(first.block.epoch==11);
  reader.seek(0,12); auto reacquired=reader.read(output);
  REQUIRE(reacquired.status==FileReadStatus::data);
  CHECK(reacquired.block.epoch==12); CHECK(reacquired.block.first_frame==0);
}

TEST_CASE("FileReader borrowed source reports open and mid-read cancellation") {
  auto source=wav(); std::stop_source early; early.request_stop();
  CHECK_THROWS_AS(FileReader(source, 1, early.get_token()),FileOpenCancelled);
  auto later=wav(); later.limit=1024; FileReader reader(later,2); std::stop_source stop; later.stop=&stop; later.cancel_on_read=true;
  std::array<float,1024> output{}; const auto result=reader.read(output,stop.get_token());
  CHECK(result.status==FileReadStatus::cancelled); CHECK(result.block.samples.empty()); CHECK_THROWS(reader.read(output));
}
TEST_CASE("FileReader borrowed source fails closed on a partial I/O error") {
  auto source=wav(); source.limit=1024; FileReader reader(source, 1); std::array<float,1024> output{};
  source.fail_after=source.calls; CHECK_THROWS_AS(reader.read(output),std::runtime_error); CHECK_THROWS(reader.read(output));
}

TEST_CASE("borrowed reader rejects zero and accepts process allocated epochs") {
  auto source=wav();
  CHECK_THROWS_AS(FileReader(source,0),std::invalid_argument); CHECK(source.calls==0);
  SourceEpochAllocator epochs; auto first=epochs.next(); auto second=epochs.next(); REQUIRE(first); REQUIRE(second);
  FileReader reader(source,*first); std::array<float,4> samples{};
  auto block=reader.read(samples); REQUIRE(block.status==FileReadStatus::data); CHECK(block.block.epoch==*first);
  const auto before=source.calls; CHECK_THROWS_AS(reader.seek(0,*first),std::invalid_argument); CHECK(source.calls==before);
  CHECK_THROWS_AS(reader.seek(0),std::logic_error); CHECK(source.calls==before);
  reader.seek(0,*second); auto after=reader.read(samples); REQUIRE(after.status==FileReadStatus::data); CHECK(after.block.epoch==*second);
}
