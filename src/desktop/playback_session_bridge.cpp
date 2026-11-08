#include "playback_session_bridge.hpp"
#ifdef _WIN32
#include "platform/windows_waveout_devices.hpp"
#include "platform/windows_playback_session.hpp"
#endif
QString PlaybackSessionBridge::state() const {
  switch (session_.state()) {
    case beatglass::PlaybackState::playing: return QStringLiteral("Playing");
    case beatglass::PlaybackState::paused: return QStringLiteral("Paused");
    default: return QStringLiteral("Stopped");
  }
}
void PlaybackSessionBridge::setDeviceId(int id) noexcept {
  if (device_id_ == id) return;
  device_id_ = id;
#ifdef _WIN32
  if (native_running_ && native_) { native_->stop(); native_running_ = false; native_error_ = QStringLiteral("Output device changed; restart native playback"); }
#endif
  emit changed();
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
bool PlaybackSessionBridge::pause() { const bool ok = session_.pause();
#ifdef _WIN32
  if (ok && native_) native_->pause();
#endif
  if (ok) emit changed(); return ok; }
void PlaybackSessionBridge::stop() { session_.stop();
#ifdef _WIN32
  if (native_) native_->stop();
#endif
  native_running_ = false;
  native_error_.clear();
  source_ = QUrl{}; emit changed(); }
bool PlaybackSessionBridge::seek(qulonglong frame, qulonglong epoch) { const bool ok = session_.seek(frame, epoch);
#ifdef _WIN32
  if (ok && native_) { native_->stop(); native_running_ = false; }
#endif
  if (ok) emit changed(); return ok; }
void PlaybackSessionBridge::refreshDevices() {
#ifdef _WIN32
  devices_.clear(); for (const auto& device : beatglass::windows::enumerate_waveout_devices()) devices_.append(QString::number(device.id) + QStringLiteral(": ") + QString::fromStdString(device.name));
#else
  devices_.clear();
#endif
  emit changed();
}
bool PlaybackSessionBridge::startNative(qulonglong epoch, int sampleRate, int channels, int maxSamples) {
#ifdef _WIN32
  if (sampleRate <= 0 || channels <= 0 || maxSamples <= 0) { native_error_ = QStringLiteral("Invalid native playback format"); emit changed(); return false; }
  if (!native_) native_ = std::make_unique<beatglass::windows::WindowsPlaybackSession>();
  std::string error; const bool ok = device_id_ >= 0 ? native_->start_device(static_cast<UINT>(device_id_), static_cast<std::uint32_t>(sampleRate), static_cast<std::uint16_t>(channels), static_cast<std::size_t>(maxSamples), epoch, error) : native_->start(static_cast<std::uint32_t>(sampleRate), static_cast<std::uint16_t>(channels), static_cast<std::size_t>(maxSamples), epoch, error);
  if (ok) { native_running_ = true; native_error_.clear(); } else native_error_ = QString::fromStdString(error);
  if (ok) emit changed(); return ok;
#else
  native_error_ = QStringLiteral("Native playback is unavailable in this build"); emit changed();
  Q_UNUSED(epoch); Q_UNUSED(sampleRate); Q_UNUSED(channels); Q_UNUSED(maxSamples); return false;
#endif
}

bool PlaybackSessionBridge::resume() { const bool ok = session_.resume(); if (ok) emit changed(); return ok; }
