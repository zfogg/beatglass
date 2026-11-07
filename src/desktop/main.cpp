#include "metadata.hpp"
#include "live_presentation.hpp"
#include "playback_guard.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef _WIN32
#include "windows_dj.hpp"
#include "windows_overlay_bridge.hpp"
#include <QQuickWindow>
#endif
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv);
  Metadata metadata;
  LivePresentation livePresentation;
  PlaybackGuard playbackGuard;
#ifdef _WIN32
  WindowsDj windowsDj;
#endif
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("playbackGuard", &playbackGuard);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
#ifdef _WIN32
  engine.rootContext()->setContextProperty("windowsDj", &windowsDj);
#endif
  engine.load(QUrl("qrc:/Main.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
#ifdef _WIN32
  auto* control = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
  auto* overlay = control ? control->findChild<QQuickWindow*>(QStringLiteral("djOverlay")) : nullptr;
  if (!control || !overlay) return 1;
  WindowsOverlay overlayBridge(overlay, control);
  overlayBridge.setOpacity(0.85);
  control->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&overlayBridge)));
#endif
  return app.exec();
}
