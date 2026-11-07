#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/byte_source.hpp"
#include <algorithm>
#include <array>
using namespace beatglass;
namespace {
class MemorySource : public ByteSource {
public:
 std::array<std::byte, 3> bytes{std::byte{1}, std::byte{2}, std::byte{3}};
 bool invalid{};
 bool premature_end{};
 std::uint64_t size() const noexcept override { return bytes.size(); }
 ByteReadResult read_at(std::uint64_t offset, std::span<std::byte> out, std::stop_token) override {
   if (invalid) return {ByteReadStatus::data, out.size() + 1};
   if (premature_end) return {ByteReadStatus::end, 0};
   auto n = std::min(out.size(), bytes.size() - static_cast<std::size_t>(offset));
   std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), n, out.begin());
   return {ByteReadStatus::data, n};
 }
};
}
TEST_CASE("byte cursor seeks and reads one borrowed source without reopening") {
 MemorySource source; ByteCursor decoder(source); std::array<std::byte, 2> out{};
 auto first = decoder.read(out); REQUIRE(first.status == ByteReadStatus::data); REQUIRE(first.bytes == 2);
 CHECK(out[0] == std::byte{1}); CHECK(out[1] == std::byte{2}); CHECK(decoder.position() == 2);
 REQUIRE(decoder.seek(0)); REQUIRE(decoder.read(out).bytes == 2);
 REQUIRE(decoder.read(out).bytes == 1); CHECK(decoder.position() == 3);
 CHECK(decoder.read(out).status == ByteReadStatus::end); CHECK_FALSE(decoder.seek(4)); CHECK(decoder.position() == 3);
}
TEST_CASE("byte cursor rejects malformed reads and preserves position on cancellation") {
 MemorySource source; ByteCursor cursor(source); std::array<std::byte, 2> out{};
 source.invalid = true; CHECK(cursor.read(out).status == ByteReadStatus::error); CHECK(cursor.position() == 0);
 source.invalid = false; source.premature_end = true;
 CHECK(cursor.read(out).status == ByteReadStatus::error); CHECK(cursor.position() == 0);
 source.premature_end = false; std::stop_source stop; stop.request_stop();
 CHECK(cursor.read(out, stop.get_token()).status == ByteReadStatus::cancelled); CHECK(cursor.position() == 0);
 CHECK(cursor.read({}).status == ByteReadStatus::error); CHECK(cursor.position() == 0);
}
