#include "playback_session_bridge.hpp"
QString PlaybackSessionBridge::state() const {
  switch (session_.state()) {
    case beatglass::PlaybackState::playing: return QStringLiteral("Playing");
    case beatglass::PlaybackState::paused: return QStringLiteral("Paused");
    default: return QStringLiteral("Stopped");
  }
}
bool PlaybackSessionBridge::open(qulonglong epoch) { const bool ok = session_.start(epoch); if (ok) emit changed(); return ok; }
bool PlaybackSessionBridge::pause() { const bool ok = session_.pause(); if (ok) emit changed(); return ok; }
void PlaybackSessionBridge::stop() { session_.stop(); emit changed(); }
bool PlaybackSessionBridge::seek(qulonglong frame, qulonglong epoch) { const bool ok = session_.seek(frame, epoch); if (ok) emit changed(); return ok; }
