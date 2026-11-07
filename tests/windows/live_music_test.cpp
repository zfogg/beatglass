#include "windows_live_music.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <deque>
#include <memory>
#ifndef _WIN32
namespace beatglass::windows {
struct Loopback::Impl {};
Loopback::Loopback() = default;
Loopback::~Loopback() = default;
bool Loopback::start(const std::string&,std::uint64_t) { return false; }
void Loopback::stop() {}
Loopback::State Loopback::state() const { return State::stopped; }
std::int32_t Loopback::error_code() const { return 0; }
bool Loopback::pop_event(Event&) { return false; }
bool Loopback::pop(LoopbackPacket&) { return false; }
std::uint64_t Loopback::dropped_frames() const { return 0; }
}
#endif
using namespace beatglass;
using namespace beatglass::windows;
namespace {
class FakeLoopback final : public Loopback {
public:
  std::uint64_t epoch{}, dropped{}; State current{State::stopped};
  std::deque<Event> events; std::deque<LoopbackPacket> packets;
  bool start(const std::string&,std::uint64_t e) override { epoch=e; dropped=0; current=State::running; events.push_back({State::running,1,e,0}); return true; }
  void stop() override { current=State::stopped; packets.clear(); events.clear(); }
  State state() const override { return current; }
  bool pop_event(Event& e) override { if(events.empty()) return false; e=events.front(); events.pop_front(); return true; }
  bool pop(LoopbackPacket& p) override { if(packets.empty()) return false; p=packets.front(); packets.pop_front(); return true; }
  std::uint64_t dropped_frames() const override { return dropped; }
  void add(std::uint64_t frame,bool discontinuity=false,bool timestamp=true) {
    LoopbackPacket p; p.epoch=epoch; p.first_frame=frame; p.sample_rate=44100; p.channels=1;
    p.sample_count=512; p.timestamp_valid=timestamp; p.discontinuity=discontinuity;
    for(unsigned i=0;i<p.sample_count;++i) p.samples[i]=0.2f*std::sin(float(frame+i)*0.037f);
    packets.push_back(p);
  }
};
}
TEST_CASE("live composite drains bounded packets and invalidates on capture loss") {
  SourceEpochAllocator epochs; auto fake=std::make_unique<FakeLoopback>(); auto* source=fake.get();
  LiveMusic live(epochs,std::move(fake)); REQUIRE(live.start("endpoint"));
  for(unsigned i=0;i<17;++i) source->add(i*512);
  REQUIRE(live.pump()); REQUIRE(live.latest()); CHECK(source->packets.size()==1);
  CHECK(live.latest()->tempo.observed_through==16*512);
  REQUIRE(live.pump()); REQUIRE(live.latest()); CHECK(live.latest()->tempo.observed_through==17*512);
  auto generation=live.generation();
  source->add(17*512,true); REQUIRE(live.pump()); CHECK(live.generation()>generation); REQUIRE(live.latest());
  generation=live.generation(); source->dropped=1; CHECK_FALSE(live.pump()); CHECK_FALSE(live.latest()); CHECK(live.generation()>generation);
  live.stop(); REQUIRE(live.start("endpoint")); CHECK(source->epoch>1);
  source->add(0,false,false); REQUIRE(live.pump()); REQUIRE(live.latest());
  live.stop(); CHECK_FALSE(live.latest());
}
TEST_CASE("live composite fails closed on malformed packet") {
  SourceEpochAllocator epochs; auto fake=std::make_unique<FakeLoopback>(); auto* source=fake.get();
  LiveMusic live(epochs,std::move(fake)); REQUIRE(live.start("endpoint")); source->add(0);
  REQUIRE(live.pump()); REQUIRE(live.latest()); source->add(512); source->packets.back().channels=0;
  CHECK_FALSE(live.pump()); CHECK_FALSE(live.latest());
}
TEST_CASE("live composite rejects delayed snapshots across gaps and silence within a packet") {
  SourceEpochAllocator epochs; auto fake=std::make_unique<FakeLoopback>(); auto* source=fake.get();
  LiveMusic live(epochs,std::move(fake)); REQUIRE(live.start("endpoint")); source->add(0);
  REQUIRE(live.pump()); REQUIRE(live.latest());
  const auto old_generation=live.generation(), old_revision=live.revision();
  source->add(1024); REQUIRE(live.pump()); REQUIRE(live.latest());
  CHECK(live.generation()>old_generation);
  // A presentation gate must keep the newest generation/revision and reject delayed data.
  auto shown_generation=live.generation(), shown_revision=live.revision();
  auto accept=[&](std::uint64_t g,std::uint64_t r) {
    if (g<shown_generation || (g==shown_generation && r<=shown_revision)) return false;
    shown_generation=g; shown_revision=r; return true;
  };
  CHECK_FALSE(accept(old_generation,old_revision));
  const auto same_generation=live.generation(), earlier_revision=live.revision();
  source->add(1536); REQUIRE(live.pump());
  REQUIRE(live.generation()==same_generation); REQUIRE(live.revision()>earlier_revision);
  REQUIRE(accept(live.generation(),live.revision())); CHECK_FALSE(accept(same_generation,earlier_revision));
  std::uint64_t frame=2048;
  for (unsigned i=0;i<86;++i) { source->add(frame); source->packets.back().samples.fill(0); frame+=512; }
  while (!source->packets.empty()) REQUIRE(live.pump());
  REQUIRE(live.latest()); REQUIRE(live.latest()->tempo.state!=TrackingState::silent);
  const auto before_silence=live.generation();
  source->add(frame); auto& packet=source->packets.back(); packet.sample_count=1024;
  for (unsigned i=0;i<512;++i) packet.samples[i]=0;
  for (unsigned i=512;i<1024;++i) packet.samples[i]=0.2f*std::sin(float(frame+i)*0.037f);
  REQUIRE(live.pump()); REQUIRE(live.latest());
  CHECK(live.latest()->tempo.state!=TrackingState::silent);
  CHECK(live.generation()>=before_silence+2);
  const auto delayed_generation=shown_generation, delayed_revision=shown_revision;
  REQUIRE(accept(live.generation(),live.revision()));
  CHECK_FALSE(accept(delayed_generation,delayed_revision));
}
