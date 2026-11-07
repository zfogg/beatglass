#include "windows_live_music_bridge.hpp"
#include <QMetaType>
#include <chrono>
#include <limits>
#include <utility>
namespace beatglass::windows {
LiveMusicBridge::LiveMusicBridge(SourceEpochAllocator& epochs,QObject* parent) : QObject(parent),epochs_(epochs) {
  qRegisterMetaType<MusicUiUpdate>();
  connect(&timer_,&QTimer::timeout,this,&LiveMusicBridge::drain);
  timer_.start(50); worker_=std::thread(&LiveMusicBridge::run,this);
}
LiveMusicBridge::~LiveMusicBridge() {
  timer_.stop(); { std::lock_guard lock(mutex_); quitting_=true; requested_=false; pending_.reset(); }
  wake_.notify_one(); if(worker_.joinable()) worker_.join();
}
bool LiveMusicBridge::advanceSession() {
  if(session_==std::numeric_limits<std::uint64_t>::max()) { exhausted_=true; return false; }
  ++session_; if(session_==std::numeric_limits<std::uint64_t>::max()) exhausted_=true;
  return true;
}
bool LiveMusicBridge::start(const QString& endpoint) {
  if(endpoint.isEmpty()) return false;
  MusicUiUpdate clear; bool accepted=false, can_clear=false;
  {
    std::lock_guard lock(mutex_);
    if(!exhausted_ && advanceSession()) {
      clear.session=session_; can_clear=true; pending_.reset();
      accepted=!exhausted_; requested_=accepted;
      if(accepted) endpoint_=endpoint.toUtf8().toStdString();
    }
  }
  if(can_clear && gate_.apply(clear)) emit musicUpdate(std::move(clear));
  wake_.notify_one(); return accepted;
}
void LiveMusicBridge::stop() {
  MusicUiUpdate clear; bool can_clear;
  {
    std::lock_guard lock(mutex_); requested_=false; pending_.reset(); can_clear=advanceSession(); clear.session=session_;
  }
  if(can_clear && gate_.apply(clear)) emit musicUpdate(std::move(clear));
  wake_.notify_one();
}
void LiveMusicBridge::drain() {
  std::optional<MusicUiUpdate> update;
  { std::lock_guard lock(mutex_); update=std::move(pending_); pending_.reset(); }
  if(update && gate_.apply(*update)) emit musicUpdate(std::move(*update));
}
void LiveMusicBridge::run() {
  LiveMusic live(epochs_); std::uint64_t active_session{}, last_revision{}; bool active=false;
  for (;;) {
    std::uint64_t session; bool requested;
    {
      std::unique_lock lock(mutex_); wake_.wait_for(lock,std::chrono::milliseconds(25));
      if(quitting_) break; session=session_; requested=requested_;
    }
    if(active && (!requested || session!=active_session)) { live.stop(); active=false; }
    if(requested && !active) {
      std::lock_guard lock(mutex_);
      if(quitting_ || !requested_ || session_!=session) continue;
      active_session=session; last_revision=0;
      active=live.start(endpoint_);
      if(!active) { requested_=false; pending_=MusicUiUpdate{session,live.generation(),live.revision(),{}}; }
    }
    if(!active) continue;
    const bool ok=live.pump();
    const auto revision=live.revision();
    if(revision!=last_revision || !ok) {
      MusicUiUpdate update{active_session,live.generation(),revision,live.latest()};
      std::lock_guard lock(mutex_);
      if(!quitting_ && requested_ && session_==active_session) {
        pending_=std::move(update); last_revision=revision;
        if(!ok) requested_=false;
      }
    }
    if(!ok) { live.stop(); active=false; }
  }
  live.stop();
}
}
