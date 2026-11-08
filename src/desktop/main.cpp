#include "metadata.hpp"
#include "live_presentation.hpp"
#include "theme_preferences.hpp"
#include "visualization_presentation_bridge.hpp"
#include "library_catalog_bridge.hpp"
#include "playback_session_bridge.hpp"
#include "analysis_presentation.hpp"
#include "capture_monitor_bridge.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef Q_OS_MACOS
#include "platform/macos_overlay.hpp"
#include <QSettings>
#endif
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; AnalysisPresentation analysis; CaptureMonitorBridge captureMonitor; LivePresentation livePresentation; VisualizationPresentationBridge visualization; ThemePreferences themePreferences; LibraryCatalogBridge libraryCatalog; PlaybackSessionBridge playbackSession;
#ifdef Q_OS_MACOS
  beatglass::macos::OverlayController macOverlayController(
      beatglass::macos::makeOverlayNative(), nullptr,
      std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass")));
#endif
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.rootContext()->setContextProperty("captureMonitor", &captureMonitor);
  engine.rootContext()->setContextProperty("analysisPresentation", &analysis);
  engine.rootContext()->setContextProperty("themePreferences", &themePreferences);
  engine.rootContext()->setContextProperty("visualizationPresentation", &visualization);
  engine.rootContext()->setContextProperty("libraryCatalog", &libraryCatalog);
  engine.rootContext()->setContextProperty("playbackSession", &playbackSession);
#ifdef Q_OS_MACOS
  engine.rootContext()->setContextProperty("macOverlayController", &macOverlayController);
#endif
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
  return app.exec();
}
