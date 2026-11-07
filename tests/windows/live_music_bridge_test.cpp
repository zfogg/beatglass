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
bool Loopback::start(const std::string&,std::uint64_t,std::shared_ptr<LoopbackStartAuthorization>) { return false; }
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
bool LiveMusic::start(const std::string&,std::shared_ptr<LoopbackStartAuthorization>) { return false; }
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
  std::atomic<bool> entered{},release{},obsolete_claimed{}; std::atomic<int> calls{};
  auto hook=[&](LiveMusic&,const std::string&,std::shared_ptr<LoopbackStartAuthorization> auth) -> bool {
    if(++calls==1) { entered=true; while(!release.load()) QThread::msleep(1); obsolete_claimed=auth->claim(); return false; }
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
  if(!error || stale_error || error_session!=3 || calls.load()!=2 || obsolete_claimed.load()) return 7;
  std::atomic<bool> destroy_entered{},destroy_cancelled{};
  auto destroying_hook=[&](LiveMusic&,const std::string&,std::shared_ptr<LoopbackStartAuthorization> auth) -> bool {
    destroy_entered=true;
    QElapsedTimer wait; wait.start();
    while(!auth->cancelled() && wait.elapsed()<5000) QThread::msleep(1);
    destroy_cancelled=auth->cancelled(); return false;
  };
  auto* destroying=new LiveMusicBridge(epochs,nullptr,destroying_hook);
  if(!destroying->start(QStringLiteral("third"))) { delete destroying; return 8; }
  timer.restart(); while(!destroy_entered && timer.elapsed()<5000) { app.processEvents(); QThread::msleep(1); }
  if(!destroy_entered) { delete destroying; return 9; }
  delete destroying;
  if(!destroy_cancelled) return 10;
  return 0;
}
