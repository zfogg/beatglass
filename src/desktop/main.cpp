#include "metadata.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
  return app.exec();
}
