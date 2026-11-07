#include "metadata.hpp"
#include "live_presentation.hpp"
#include "playback_guard.hpp"
#include "app_lifecycle.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQuickWindow>
#include <QTextStream>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef Q_OS_MACOS
#include "platform/macos_overlay.hpp"
#include <QSettings>
#include <memory>
#endif
#ifdef BEATGLASS_EXPERIMENTAL_COLLECTION
#include "collection_view.hpp"
#include <memory>
#endif
#ifdef _WIN32
#include "windows_dj.hpp"
#include "windows_overlay_bridge.hpp"
#include <QQuickWindow>
#include <QSettings>
#endif
namespace {
int reportStartupFailure(QMessageBox& dialog) {
  const auto platform = QGuiApplication::platformName();
  if (AppLifecycle::useNoninteractiveErrors(platform, qEnvironmentVariableIntValue("BEATGLASS_NONINTERACTIVE") != 0)) {
    QTextStream error(stderr); error << dialog.text() << '\n' << dialog.informativeText() << '\n'; error.flush();
  } else {
    dialog.exec();
  }
  return 1;
}
}
int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("Beatglass")); app.setApplicationName(QStringLiteral("Beatglass"));
  app.setQuitOnLastWindowClosed(false);
  auto dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#ifdef BEATGLASS_TEST_STARTUP_PROBE
  const auto testDirectory = qEnvironmentVariable("BEATGLASS_TEST_DATA_DIR");
  if (!testDirectory.isEmpty()) dataDirectory = testDirectory;
#endif
  if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory)) {
    auto dialog = AppLifecycle::handoffFailureDialog();
    dialog->setText(QObject::tr("Beatglass could not access its application data folder."));
    dialog->setInformativeText(QObject::tr("Check that your user account can write to its application data folder, then try again."));
    return reportStartupFailure(*dialog);
  }
  AppLifecycle lifecycle(QStringLiteral("desktop"), dataDirectory);
  const auto startup = lifecycle.start();
  if (startup == AppLifecycle::Startup::restored) return 0;
  if (startup == AppLifecycle::Startup::unavailable) { auto dialog = AppLifecycle::handoffFailureDialog(); return reportStartupFailure(*dialog); }
#ifdef BEATGLASS_TEST_STARTUP_PROBE
  return 0;
#else
  beatglass::SourceEpochAllocator epochs;
  Metadata metadata;
  LivePresentation livePresentation;
  PlaybackGuard playbackGuard;
#ifdef Q_OS_MACOS
  beatglass::macos::OverlayController macOverlayController(
      beatglass::macos::makeOverlayNative(), nullptr,
      std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass")));
#endif
#ifdef _WIN32
  WindowsDj windowsDj(epochs);
  QSettings overlaySettings(QStringLiteral("Beatglass"), QStringLiteral("Beatglass"));
#endif
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("playbackGuard", &playbackGuard);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.rootContext()->setContextProperty("appLifecycle", &lifecycle);
#ifdef Q_OS_MACOS
  engine.rootContext()->setContextProperty("macOverlayController", &macOverlayController);
#endif
#ifdef BEATGLASS_EXPERIMENTAL_COLLECTION
  std::unique_ptr<CollectionView> collection;
  auto database = qEnvironmentVariable("BEATGLASS_EXPERIMENTAL_LIBRARY_DB");
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  if (database.isEmpty()) database = QStringLiteral(":memory:");
#endif
  if (!database.isEmpty()) {
    collection = std::make_unique<CollectionView>(database, epochs);
    engine.rootContext()->setContextProperty("experimentalCollection", collection.get());
  }
#endif
#ifdef _WIN32
  engine.rootContext()->setContextProperty("windowsDj", &windowsDj);
#endif
  engine.load(QUrl("qrc:/Main.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
  auto* control = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
  if (!control) return 1;
#ifdef _WIN32
  auto* overlay = control ? control->findChild<QQuickWindow*>(QStringLiteral("djOverlay")) : nullptr;
  if (!control || !overlay) return 1;
  WindowsOverlay overlayBridge(overlay, control, nullptr, &overlaySettings);
  control->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&overlayBridge)));
  QObject::connect(&lifecycle, &AppLifecycle::recoveryAvailableChanged, &overlayBridge, [&] { overlayBridge.setExternalRecoveryAvailable(lifecycle.recoveryAvailable()); });
#endif
#ifdef Q_OS_MACOS
  QObject::connect(&lifecycle, &AppLifecycle::recoveryAvailableChanged, &macOverlayController, [&] { macOverlayController.setExternalRecoveryAvailable(lifecycle.recoveryAvailable()); });
#endif
  lifecycle.attach(control);
  return app.exec();
#endif
}
