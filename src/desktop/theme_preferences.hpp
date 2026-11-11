#pragma once
#include <QObject>
#include <QSettings>
#include <QString>
#include <memory>

/** GUI-thread-only application preferences for the desktop presentation. The bridge owns its QSettings store, reads the saved theme index on construction, and writes validated changes immediately; persistence failures are not currently surfaced to QML. */
class ThemePreferences final : public QObject {
  Q_OBJECT
  Q_PROPERTY(int themeIndex READ themeIndex WRITE setThemeIndex NOTIFY themeIndexChanged)
  Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY reduceMotionChanged)
  Q_PROPERTY(QString lowColor READ lowColor WRITE setLowColor NOTIFY visualizationColorsChanged)
  Q_PROPERTY(QString midColor READ midColor WRITE setMidColor NOTIFY visualizationColorsChanged)
  Q_PROPERTY(QString highColor READ highColor WRITE setHighColor NOTIFY visualizationColorsChanged)
public:
  /** Construct preferences with an optional settings store; the default uses Beatglass user-scope settings. */
  explicit ThemePreferences(QObject* parent = nullptr, std::unique_ptr<QSettings> settings = {});
  /** Return the persisted theme index, clamped to the six supported themes. */
  int themeIndex() const noexcept { return theme_index_; }
  /** Store a theme index after clamping it to the supported range. */
  void setThemeIndex(int index);
  /** Return the persisted reduced-motion preference. */
  bool reduceMotion() const noexcept { return reduce_motion_; }
  /** Store the reduced-motion preference. */
  void setReduceMotion(bool value);
  /** Return the persisted low-frequency visualization color as a CSS color. */
  QString lowColor() const { return low_color_; }
  /** Store a valid low-frequency visualization color as a CSS color. */
  void setLowColor(const QString& value);
  /** Return the persisted mid-frequency visualization color as a CSS color. */
  QString midColor() const { return mid_color_; }
  /** Store a valid mid-frequency visualization color as a CSS color. */
  void setMidColor(const QString& value);
  /** Return the persisted high-frequency visualization color as a CSS color. */
  QString highColor() const { return high_color_; }
  /** Store a valid high-frequency visualization color as a CSS color. */
  void setHighColor(const QString& value);
  /** Restore the portable presentation defaults and persist both values. */
  Q_INVOKABLE void reset();
signals:
  /** Emitted after a new clamped theme index is stored. */
  void themeIndexChanged();
  /** Emitted after the reduced-motion preference changes. */
  void reduceMotionChanged();
  /** Emitted after any visualization band color changes. */
  void visualizationColorsChanged();
private:
  std::unique_ptr<QSettings> settings_;
  int theme_index_ = 0;
  bool reduce_motion_ = false;
  QString low_color_ = QStringLiteral("#38bdf8");
  QString mid_color_ = QStringLiteral("#a78bfa");
  QString high_color_ = QStringLiteral("#f472b6");
};
