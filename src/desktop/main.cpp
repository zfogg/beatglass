#include "metadata.hpp"
#include "live_presentation.hpp"
#include "theme_preferences.hpp"
#include "visualization_presentation_bridge.hpp"
#include "library_catalog_bridge.hpp"
#include "library_preferences.hpp"
#include "playback_session_bridge.hpp"
#include "analysis_presentation.hpp"
#include "capture_monitor_bridge.hpp"
#include "storage_recovery_bridge.hpp"
#ifdef Q_OS_WINDOWS
#include "platform/windows_endpoint_bridge.hpp"
#include "platform/windows_overlay.hpp"
#endif
#ifdef BEATGLASS_BUILD_DESKTOP_ANALYSIS
#include "analysis_job_bridge.hpp"
#include "beatglass/library/detail/job_executor.hpp"
#endif
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <memory>
#ifdef Q_OS_MACOS
#include "platform/macos_overlay.hpp"
#include <QSettings>
#endif
int main(int argc, char** argv) {
#ifdef Q_OS_WINDOWS
  beatglass::windows::EndpointBridge endpointSelection;
#endif
  QGuiApplication app(argc, argv); Metadata metadata; AnalysisPresentation analysis;
  StorageRecoveryBridge storageRecovery;
  const QStringList args = app.arguments();
  const int storageIndex = args.indexOf(QStringLiteral("--database"));
  if (storageIndex >= 0 && storageIndex + 1 < args.size()) storageRecovery.setDatabasePath(args.at(storageIndex + 1));
  if (!storageRecovery.databasePath().isEmpty()) storageRecovery.open();
#ifdef BEATGLASS_BUILD_DESKTOP_ANALYSIS
  beatglass::library_detail::JobExecutor analysisExecutor(2); AnalysisJobBridge analysisJob(analysisExecutor);
#endif
  CaptureMonitorBridge captureMonitor; LibraryPreferences libraryPreferences; LivePresentation livePresentation; VisualizationPresentationBridge visualization; ThemePreferences themePreferences; LibraryCatalogBridge libraryCatalog; PlaybackSessionBridge playbackSession;
#ifdef Q_OS_MACOS
  beatglass::macos::OverlayController macOverlayController(
      beatglass::macos::makeOverlayNative(), nullptr,
      std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass")));
#endif
  QQmlApplicationEngine engine;
#ifdef Q_OS_WINDOWS
  engine.rootContext()->setContextProperty("endpointSelection", &endpointSelection);
#endif
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.rootContext()->setContextProperty("captureMonitor", &captureMonitor);
  engine.rootContext()->setContextProperty("analysisPresentation", &analysis);
#ifdef BEATGLASS_BUILD_DESKTOP_ANALYSIS
  engine.rootContext()->setContextProperty("analysisJob", &analysisJob);
#endif
  engine.rootContext()->setContextProperty("themePreferences", &themePreferences);
  engine.rootContext()->setContextProperty("visualizationPresentation", &visualization);
  engine.rootContext()->setContextProperty("libraryCatalog", &libraryCatalog);
  engine.rootContext()->setContextProperty("libraryPreferences", &libraryPreferences);
  engine.rootContext()->setContextProperty("playbackSession", &playbackSession);
  engine.rootContext()->setContextProperty("storageRecovery", &storageRecovery);
#ifdef Q_OS_MACOS
  engine.rootContext()->setContextProperty("macOverlayController", &macOverlayController);
#endif
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
#ifdef Q_OS_WINDOWS
  // The QML overlay is created by the engine, so wire the native controller
  // after loading while keeping ownership in this GUI-thread scope.
  auto* root = engine.rootObjects().constFirst();
  auto* control = qobject_cast<QQuickWindow*>(root);
  auto* overlay = root->findChild<QQuickWindow*>(QStringLiteral("djOverlay"));
  std::unique_ptr<QSettings> overlaySettings = std::make_unique<QSettings>(QSettings::NativeFormat,
      QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass"));
  beatglass::windows::OverlayController windowsOverlay(overlay, control, nullptr, overlaySettings.get());
  root->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&windowsOverlay)));
#endif
  return app.exec();
}
