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
  devices_.clear(); bool selected_present = device_id_ < 0;
  for (const auto& device : beatglass::windows::enumerate_waveout_devices()) { if (static_cast<int>(device.id) == device_id_) selected_present = true; devices_.append(QString::number(device.id) + QStringLiteral(": ") + QString::fromStdString(device.name)); }
  if (!selected_present && native_running_) { if (native_) native_->stop(); native_running_ = false; native_error_ = QStringLiteral("Selected output disappeared; choose another device and restart"); }
#else
  devices_.clear();
#endif
  emit changed();
}
bool PlaybackSessionBridge::startNative(qulonglong epoch, int sampleRate, int channels, int maxSamples) {
  native_epoch_ = epoch; native_rate_ = sampleRate; native_channels_ = channels; native_max_samples_ = maxSamples;
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
bool PlaybackSessionBridge::retryNative() { if (!native_epoch_ || native_rate_ <= 0 || native_channels_ <= 0 || native_max_samples_ <= 0) return false; return startNative(native_epoch_, native_rate_, native_channels_, native_max_samples_); }

bool PlaybackSessionBridge::resume() { const bool ok = session_.resume(); if (ok) emit changed(); return ok; }
