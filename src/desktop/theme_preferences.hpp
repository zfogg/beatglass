#pragma once
#include <QObject>
#include <QSettings>
#include <memory>

/** GUI-thread-only application preferences for the desktop presentation. The bridge owns its QSettings store, reads the saved theme index on construction, and writes validated changes immediately; persistence failures are not currently surfaced to QML. */
class ThemePreferences final : public QObject {
  Q_OBJECT
  Q_PROPERTY(int themeIndex READ themeIndex WRITE setThemeIndex NOTIFY themeIndexChanged)
public:
  /** Construct preferences with an optional settings store; the default uses Beatglass user-scope settings. */
  explicit ThemePreferences(QObject* parent = nullptr, std::unique_ptr<QSettings> settings = {});
  /** Return the persisted theme index, clamped to the six supported themes. */
  int themeIndex() const noexcept { return theme_index_; }
  /** Store a theme index after clamping it to the supported range. */
  void setThemeIndex(int index);
signals:
  /** Emitted after a new clamped theme index is stored. */
  void themeIndexChanged();
private:
  std::unique_ptr<QSettings> settings_;
  int theme_index_ = 0;
};
