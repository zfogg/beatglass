#include "metadata.hpp"
#include "live_presentation.hpp"
#include "playback_guard.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <QGuiApplication>
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
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
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
#ifdef _WIN32
  auto* control = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
  auto* overlay = control ? control->findChild<QQuickWindow*>(QStringLiteral("djOverlay")) : nullptr;
  if (!control || !overlay) return 1;
  WindowsOverlay overlayBridge(overlay, control, nullptr, &overlaySettings);
  control->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&overlayBridge)));
#endif
  return app.exec();
}
