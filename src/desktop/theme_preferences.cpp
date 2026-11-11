#include "theme_preferences.hpp"
#include <algorithm>
#include <QColor>

ThemePreferences::ThemePreferences(QObject* parent, std::unique_ptr<QSettings> settings)
    : QObject(parent), settings_(std::move(settings)) {
  if (!settings_) {
    settings_ = std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope,
                                            QStringLiteral("Beatglass"), QStringLiteral("Beatglass"));
  }
  setThemeIndex(settings_->value(QStringLiteral("desktop/themeIndex"), 0).toInt());
  setReduceMotion(settings_->value(QStringLiteral("desktop/reduceMotion"), false).toBool());
  setLowColor(settings_->value(QStringLiteral("visualization/lowColor"), low_color_).toString());
  setMidColor(settings_->value(QStringLiteral("visualization/midColor"), mid_color_).toString());
  setHighColor(settings_->value(QStringLiteral("visualization/highColor"), high_color_).toString());
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

namespace {
QString normalized_color(const QString& value, const QString& fallback) {
  const QColor color(value);
  return color.isValid() ? color.name(QColor::HexRgb) : fallback;
}
}

void ThemePreferences::setLowColor(const QString& value) {
  const QString color = normalized_color(value, low_color_);
  if (low_color_ == color) return;
  low_color_ = color;
  if (settings_) settings_->setValue(QStringLiteral("visualization/lowColor"), low_color_);
  emit visualizationColorsChanged();
}

void ThemePreferences::setMidColor(const QString& value) {
  const QString color = normalized_color(value, mid_color_);
  if (mid_color_ == color) return;
  mid_color_ = color;
  if (settings_) settings_->setValue(QStringLiteral("visualization/midColor"), mid_color_);
  emit visualizationColorsChanged();
}

void ThemePreferences::setHighColor(const QString& value) {
  const QString color = normalized_color(value, high_color_);
  if (high_color_ == color) return;
  high_color_ = color;
  if (settings_) settings_->setValue(QStringLiteral("visualization/highColor"), high_color_);
  emit visualizationColorsChanged();
}

void ThemePreferences::reset() {
  setThemeIndex(0);
  setReduceMotion(false);
  setLowColor(QStringLiteral("#38bdf8"));
  setMidColor(QStringLiteral("#a78bfa"));
  setHighColor(QStringLiteral("#f472b6"));
  // Re-write defaults even when the in-memory value was already default; this
  // repairs a settings store containing an invalid or stale representation.
  if (settings_) {
    settings_->setValue(QStringLiteral("desktop/themeIndex"), 0);
    settings_->setValue(QStringLiteral("desktop/reduceMotion"), false);
    settings_->setValue(QStringLiteral("visualization/lowColor"), low_color_);
    settings_->setValue(QStringLiteral("visualization/midColor"), mid_color_);
    settings_->setValue(QStringLiteral("visualization/highColor"), high_color_);
    settings_->sync();
  }
}
