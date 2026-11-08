#include "playback_session_bridge.hpp"
QString PlaybackSessionBridge::state() const {
  switch (session_.state()) {
    case beatglass::PlaybackState::playing: return QStringLiteral("Playing");
    case beatglass::PlaybackState::paused: return QStringLiteral("Paused");
    default: return QStringLiteral("Stopped");
  }
}
bool PlaybackSessionBridge::open(qulonglong epoch) { const bool ok = session_.start(epoch); if (ok) emit changed(); return ok; }
bool PlaybackSessionBridge::openSource(const QUrl& source, qulonglong epoch) {
  if (!source.isValid() || source.isEmpty()) return false;
  const bool ok = session_.start(epoch); if (ok) { source_ = source; emit changed(); } return ok;
}
bool PlaybackSessionBridge::replaceSource(const QUrl& source, qulonglong epoch) {
  if (!source.isValid() || source.isEmpty() || !session_.replace(epoch)) return false;
  source_ = source; emit changed(); return true;
}
bool PlaybackSessionBridge::pause() { const bool ok = session_.pause(); if (ok) emit changed(); return ok; }
void PlaybackSessionBridge::stop() { session_.stop(); source_ = {}; emit changed(); }
bool PlaybackSessionBridge::seek(qulonglong frame, qulonglong epoch) { const bool ok = session_.seek(frame, epoch); if (ok) emit changed(); return ok; }
