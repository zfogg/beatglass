#include "playback_guard.hpp"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QtGlobal>

bool PlaybackGuard::invalidChannelMask(std::uint32_t mask) noexcept {
  return mask == 0 || (mask & (std::uint32_t{1} << QAudioFormat::UnknownPosition)) != 0;
}
QString PlaybackGuard::diagnostic(const QAudioDevice& device) const {
  if (device.isNull()) return QStringLiteral("No playback output is available.");
#ifdef Q_OS_WIN
  const auto backend = qgetenv("QT_MEDIA_BACKEND");
  if ((backend.isEmpty() || backend == "ffmpeg") && invalidChannelMask(static_cast<std::uint32_t>(device.channelConfiguration())))
    return QStringLiteral("This output reports an unsupported channel layout for the FFmpeg playback backend. Choose another playback output.");
#endif
  return {};
}
