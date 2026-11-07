#ifndef Q_MOC_RUN
#include "beatglass/core/file_analysis.hpp"
#endif
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QObject>
#include <QUrl>
class Metadata : public QObject {
  Q_OBJECT
public:
  Q_INVOKABLE QString inspect(const QUrl& url) {
    if (!url.isLocalFile()) return "Select a local file.";
    try {
#ifdef _WIN32
      const auto path = std::filesystem::path(url.toLocalFile().toStdWString());
#else
      const auto path = std::filesystem::path(url.toLocalFile().toUtf8().constData());
#endif
      const auto m = beatglass::read_metadata(path);
      return QString("%1 · %2 Hz · %3 channels · %4 seconds").arg(QString::fromStdString(m.format)).arg(m.sample_rate).arg(m.channels).arg(double(m.frames)/m.sample_rate, 0, 'f', 2);
    } catch (const std::exception& e) { return QString("Error: %1").arg(e.what()); }
  }
};
int main(int argc, char** argv) {
  QGuiApplication app(argc, argv); Metadata metadata; QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("metadata", &metadata);
  engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
  return app.exec();
}
#include "main.moc"
