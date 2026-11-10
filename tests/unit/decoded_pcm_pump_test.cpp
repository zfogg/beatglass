#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "beatglass/capture/decoded_pcm_pump.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>
using namespace beatglass;
namespace {
struct WaveFixture {
  std::filesystem::path path;
  WaveFixture() {
    path = std::filesystem::temp_directory_path() / ("beatglass-pump-" + std::to_string(std::random_device{}()) + ".wav");
    std::ofstream out(path, std::ios::binary);
    auto text=[&](const char* p,int n){out.write(p,n);}; auto u16=[&](unsigned n){char b[2]={char(n&255),char(n>>8)};out.write(b,2);}; auto u32=[&](unsigned n){u16(n);u16(n>>16);};
    text("RIFF",4);u32(52);text("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(8000);u32(16000);u16(2);u16(16);text("data",4);u32(8);u16(0);u16(32767);u16(16384);u16(8192);
  }
  ~WaveFixture(){std::error_code e;std::filesystem::remove(path,e);}
};
}
TEST_CASE("decoded PCM pump hands every block to the output sink") {
  WaveFixture fixture; FileReader reader(fixture.path, 12); std::array<float,2> buffer{}; std::vector<float> seen; std::vector<std::uint64_t> frames;
  const auto status = pump_decoded_pcm(reader, buffer, [&](const AudioBlock& block){ frames.push_back(block.first_frame); seen.insert(seen.end(), block.samples.begin(), block.samples.end()); });
  REQUIRE(status == DecodedPcmPumpStatus::end); CHECK(frames == std::vector<std::uint64_t>{0,2}); REQUIRE(seen.size() == 4); CHECK(seen[1] == Catch::Approx(32767.0/32768));
}
TEST_CASE("decoded PCM pump observes cancellation between pulls") {
  WaveFixture fixture; FileReader reader(fixture.path, 4); std::array<float,2> buffer{}; std::stop_source stop; std::size_t calls=0;
  CHECK(pump_decoded_pcm(reader, buffer, [&](const AudioBlock&){ ++calls; stop.request_stop(); }, stop.get_token()) == DecodedPcmPumpStatus::cancelled); CHECK(calls == 1);
}
