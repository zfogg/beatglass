#include "metadata.hpp"
#include "live_presentation.hpp"
#include "playback_guard.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; LivePresentation livePresentation; PlaybackGuard playbackGuard; QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("playbackGuard", &playbackGuard);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
  return app.exec();
}
