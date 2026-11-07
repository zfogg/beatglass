#include "metadata.hpp"
#include "live_presentation.hpp"
#include "theme_preferences.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef Q_OS_MACOS
#include "platform/macos_overlay.hpp"
#include <QSettings>
#endif
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; LivePresentation livePresentation; ThemePreferences themePreferences;
#ifdef Q_OS_MACOS
  beatglass::macos::OverlayController macOverlayController(
      beatglass::macos::makeOverlayNative(), nullptr,
      std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass")));
#endif
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.rootContext()->setContextProperty("themePreferences", &themePreferences);
#ifdef Q_OS_MACOS
  engine.rootContext()->setContextProperty("macOverlayController", &macOverlayController);
#endif
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
  return app.exec();
}
