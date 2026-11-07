#include "metadata.hpp"
#include "live_presentation.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef Q_OS_WIN
#include "platform/windows_dj_bridge.hpp"
#include <QVariant>
#include <utility>
#endif
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; LivePresentation livePresentation;
#ifdef Q_OS_WIN
  beatglass::windows::DjBridge djBridge;
  QObject::connect(&djBridge, &beatglass::windows::DjBridge::presentationUpdate, &livePresentation,
                   [&livePresentation](beatglass::PresentationUpdate update) { livePresentation.apply(std::move(update)); });
#endif
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.rootContext()->setContextProperty("livePresentation", &livePresentation);
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
#ifdef Q_OS_WIN
  engine.rootObjects().first()->setProperty("djControl", QVariant::fromValue(static_cast<QObject*>(&djBridge)));
#endif
  return app.exec();
}
