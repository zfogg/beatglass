#include "theme_preferences.hpp"
#include <algorithm>

ThemePreferences::ThemePreferences(QObject* parent, std::unique_ptr<QSettings> settings)
    : QObject(parent), settings_(std::move(settings)) {
  if (!settings_) {
    settings_ = std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope,
                                            QStringLiteral("Beatglass"), QStringLiteral("Beatglass"));
  }
  setThemeIndex(settings_->value(QStringLiteral("desktop/themeIndex"), 0).toInt());
  setReduceMotion(settings_->value(QStringLiteral("desktop/reduceMotion"), false).toBool());
}

void ThemePreferences::setReduceMotion(bool value) {
  if (reduce_motion_ == value) return;
  reduce_motion_ = value;
  if (settings_) settings_->setValue(QStringLiteral("desktop/reduceMotion"), reduce_motion_);
  emit reduceMotionChanged();
}

void ThemePreferences::setThemeIndex(int index) {
  const int clamped = std::clamp(index, 0, 5);
  if (theme_index_ == clamped) return;
  theme_index_ = clamped;
  if (settings_) settings_->setValue(QStringLiteral("desktop/themeIndex"), theme_index_);
  emit themeIndexChanged();
}
