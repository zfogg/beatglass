#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/audio_queue.hpp"
#include "beatglass/core/strategies.hpp"
#include "beatglass/core/presentation.hpp"
#include "beatglass/capture/source.hpp"
#include "beatglass/library/store.hpp"
#include <type_traits>
#include <array>
#include <atomic>
#include <thread>
#include <limits>
using namespace beatglass;
TEST_CASE("queue rejects blocks whose frame range overflows") {
  AudioQueue<3, 2> q; std::array<float,1> samples{1};
  AudioBlock input{{44100,1},1,std::numeric_limits<std::uint64_t>::max(),0,false,samples};
  CHECK_FALSE(q.push(input)); CHECK(q.dropped_frames()==1); CHECK(q.size()==0);
}
TEST_CASE("queue rejects blocks without a source epoch") {
  AudioQueue<3, 4> q; std::array<float, 1> samples{1};
  AudioBlock input{{44100, 1}, 0, 0, 0, false, samples};
  CHECK_FALSE(q.push(input));
  CHECK(q.dropped_frames() == 1);
}
TEST_CASE("queue preserves data and wraps") {
  AudioQueue<3, 4> q; AudioQueue<3, 4>::Block out;
  std::array<float, 4> samples{1, 2, 3, 4};
  AudioBlock in{{48000, 2}, 7, 10, 123, false, samples};
  REQUIRE(q.push(in)); samples[0] = 9;
  REQUIRE(q.pop(out)); CHECK(out.samples[0] == 1); CHECK(out.first_frame == 10);
  REQUIRE(q.push(in)); REQUIRE(q.pop(out)); REQUIRE(q.push(in)); REQUIRE(q.pop(out));
  CHECK_FALSE(q.pop(out));
}
TEST_CASE("overflow drops complete blocks and marks the next block") {
  AudioQueue<2, 4> q; AudioQueue<2, 4>::Block out;
  std::array<float, 4> samples{}; AudioBlock in{{44100, 2}, 1, 0, 0, false, samples};
  REQUIRE(q.push(in)); CHECK_FALSE(q.push(in)); CHECK(q.dropped_frames() == 2);
  REQUIRE(q.pop(out)); CHECK_FALSE(out.discontinuity);
  in.first_frame = 4; REQUIRE(q.push(in)); REQUIRE(q.pop(out));
  CHECK(out.discontinuity); CHECK(out.first_frame == 4);
}
TEST_CASE("malformed blocks cannot enter queue") {
  AudioQueue<2, 2> q; AudioQueue<2, 2>::Block out;
  std::array<float, 3> samples{}; AudioBlock in{{48000, 2}, 1, 0, 0, false, samples};
  CHECK_FALSE(q.push(in)); CHECK_FALSE(q.pop(out));
  std::array<float, 2> valid{}; in.samples = valid;
  REQUIRE(q.push(in)); REQUIRE(q.pop(out)); CHECK(out.discontinuity);
}

TEST_CASE("empty and zero-channel blocks fail closed and mark reacquisition") {
  AudioQueue<3, 4> q;
  AudioQueue<3, 4>::Block out;

  AudioBlock empty{{48000, 2}, 9, 100, 1234, false, std::array<float, 0>{}};
  CHECK_FALSE(q.push(empty));
  CHECK(q.dropped_frames() == 0);

  AudioBlock zero_channels{{48000, 0}, 9, 100, 1234, false,
                           std::array<float, 2>{1.0F, 2.0F}};
  CHECK_FALSE(q.push(zero_channels));
  CHECK(q.dropped_frames() == 0);

  AudioBlock valid{{48000, 2}, 9, 100, 1234, false,
                   std::array<float, 2>{3.0F, 4.0F}};
  REQUIRE(q.push(valid));
  REQUIRE(q.pop(out));
  CHECK(out.discontinuity);
  CHECK(out.epoch == 9);
  CHECK(out.first_frame == 100);
  CHECK(out.monotonic_ns == 1234);
}

TEST_CASE("explicit producer discontinuity is preserved on accepted blocks") {
  AudioQueue<3, 4> q;
  AudioQueue<3, 4>::Block out;
  AudioBlock discontinuous{{48000, 1}, 12, 44, 9876, true,
                           std::array<float, 1>{0.5F}};

  REQUIRE(q.push(discontinuous));
  REQUIRE(q.pop(out));
  CHECK(out.discontinuity);
  CHECK(out.epoch == 12);
  CHECK(out.first_frame == 44);
  CHECK(out.monotonic_ns == 9876);
}

TEST_CASE("a valid oversized block is dropped as complete frames") {
  AudioQueue<2, 2> q; AudioQueue<2, 2>::Block out;
  std::array<float, 4> oversized{1, 2, 3, 4};
  AudioBlock in{{48000, 2}, 2, 0, 0, false, oversized};
  CHECK_FALSE(q.push(in)); CHECK(q.dropped_frames() == 2); CHECK_FALSE(q.pop(out));
  std::array<float, 2> valid{5, 6}; in.samples = valid; in.first_frame = 2;
  REQUIRE(q.push(in)); REQUIRE(q.pop(out));
  CHECK(out.discontinuity); CHECK(out.first_frame == 2); CHECK(out.samples[0] == 5);
}
TEST_CASE("concurrent producer and consumer preserve order through wraparound") {
  constexpr std::uint64_t count = 100000;
  AudioQueue<32, 1> q;
  std::atomic<bool> start{false};
  std::atomic<bool> ordering_error{false};
  std::thread producer([&] {
    while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
    for (std::uint64_t i = 0; i < count; ++i) {
      const std::array<float, 1> sample{static_cast<float>(i)};
      AudioBlock block{{48000, 1}, 42, i, 0, false, sample};
      while (!q.push(block)) std::this_thread::yield();
    }
  });
  std::thread consumer([&] {
    AudioQueue<32, 1>::Block block;
    start.store(true, std::memory_order_release);
    for (std::uint64_t i = 0; i < count; ++i) {
      while (!q.pop(block)) std::this_thread::yield();
      if (block.epoch != 42 || block.first_frame != i || block.sample_count != 1 ||
          block.samples[0] != static_cast<float>(i)) ordering_error.store(true);
    }
  });
  producer.join(); consumer.join();
  CHECK_FALSE(ordering_error.load());
}

TEST_CASE("foundation ports cannot be instantiated without adapters") {
  CHECK(std::is_abstract_v<AudioSource>);
  CHECK(std::is_abstract_v<TempoEstimator>);
  CHECK(std::is_abstract_v<KeyEstimator>);
  CHECK(std::is_abstract_v<ChordEstimator>);
  CHECK(std::is_abstract_v<AnalysisStore>);
  CHECK(std::is_abstract_v<PresentationAdapter>);
}
