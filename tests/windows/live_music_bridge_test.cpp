#include "windows_live_music_bridge.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QThread>
#include <atomic>
#include <limits>
#include <stdexcept>
#ifdef BEATGLASS_BRIDGE_TEST_STUB
namespace beatglass {
struct CausalMusicReplay::Impl {};
CausalMusicReplay::~CausalMusicReplay() = default;
}
namespace beatglass::windows {
struct Loopback::Impl {};
Loopback::Loopback() = default;
Loopback::~Loopback() = default;
bool Loopback::start(const std::string&,std::uint64_t) { return false; }
void Loopback::stop() {}
Loopback::State Loopback::state() const { return State::stopped; }
bool Loopback::pop_event(Event&) { return false; }
bool Loopback::pop(LoopbackPacket&) { return false; }
std::uint64_t Loopback::dropped_frames() const { return 0; }
LoopbackSession::LoopbackSession(SourceEpochAllocator& e,std::function<void()> f) : epochs_(e),invalidate_(std::move(f)) {}
LoopbackSession::~LoopbackSession() = default;
LiveMusic::LiveMusic(SourceEpochAllocator& e) : session_(e,{}) {}
LiveMusic::~LiveMusic() = default;
bool LiveMusic::start(const std::string&) { return false; }
bool LiveMusic::pump() { return false; }
void LiveMusic::stop() {}
}
#endif
using namespace beatglass;
using namespace beatglass::windows;
int main(int argc,char** argv) {
  QCoreApplication app(argc,argv); SourceEpochAllocator epochs;
  {
    QPointer<LiveMusicBridge> bridge=new LiveMusicBridge(epochs);
    QObject::connect(bridge,&LiveMusicBridge::musicUpdate,bridge,[&](const MusicUiUpdate&){ delete bridge.data(); },Qt::DirectConnection);
    auto* raw=bridge.data(); if(!raw->start(QStringLiteral("endpoint")) || bridge) return 1;
  }
  {
    QPointer<LiveMusicBridge> bridge=new LiveMusicBridge(epochs);
    QObject::connect(bridge,&LiveMusicBridge::musicUpdate,bridge,[&](const MusicUiUpdate&){ delete bridge.data(); },Qt::DirectConnection);
    auto* raw=bridge.data(); raw->stop(); if(bridge) return 2;
  }
  std::atomic<bool> entered{},release{}; std::atomic<int> calls{};
  auto hook=[&](LiveMusic&,const std::string&) -> bool {
    if(++calls==1) { entered=true; while(!release.load()) QThread::msleep(1); }
    throw std::runtime_error("injected start failure");
  };
  LiveMusicBridge bridge(epochs,nullptr,hook);
  bool error=false, stale_error=false; std::uint64_t error_session{};
  QObject::connect(&bridge,&LiveMusicBridge::musicUpdate,&bridge,[&](const MusicUiUpdate& update){
    if(update.error) { if(update.session!=3) stale_error=true; error=true; error_session=update.session; }
  },Qt::DirectConnection);
  if(!bridge.start(QStringLiteral("first"))) return 3;
  QElapsedTimer timer; timer.start(); while(!entered && timer.elapsed()<5000) { app.processEvents(); QThread::msleep(1); }
  if(!entered) { release=true; return 4; }
  timer.restart(); bridge.stop(); if(timer.elapsed()>500) { release=true; return 5; }
  if(!bridge.start(QStringLiteral("second"))) { release=true; return 6; }
  release=true; timer.restart();
  while(!error && timer.elapsed()<5000) { app.processEvents(); QThread::msleep(1); }
  if(!error || stale_error || error_session!=3 || calls.load()!=2) return 7;
  return 0;
}
