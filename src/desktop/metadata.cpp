#include "metadata.hpp"
#include "beatglass/core/file_analysis.hpp"
#include <QMetaObject>
#include <QThreadPool>
#include <mutex>
#include <utility>
struct Metadata::State { std::mutex mutex; Metadata* owner = nullptr; };
namespace {
QString readFile(const QUrl& url) {
  if (!url.isLocalFile()) return QStringLiteral("Select a local file.");
  try {
#ifdef _WIN32
    const auto path = std::filesystem::path(url.toLocalFile().toStdWString());
#else
    const auto path = std::filesystem::path(url.toLocalFile().toUtf8().constData());
#endif
    const auto m = beatglass::read_metadata(path);
    return QStringLiteral("%1 | %2 Hz | %3 channels | %4 seconds").arg(QString::fromStdString(m.format)).arg(m.sample_rate).arg(m.channels).arg(double(m.frames)/m.sample_rate, 0, 'f', 2);
  } catch (const std::exception& e) { return QStringLiteral("Error: %1").arg(QString::fromUtf8(e.what())); }
}
}
Metadata::Metadata(QObject* parent, Reader reader) : QObject(parent), state_(std::make_shared<State>()), reader_(reader ? std::move(reader) : Reader(readFile)) { state_->owner = this; }
Metadata::~Metadata() { std::lock_guard lock(state_->mutex); state_->owner = nullptr; }
void Metadata::inspect(const QUrl& url) {
  ++generation_;
  emit statusChanged(QStringLiteral("Reading metadata..."));
  if (running_) { pending_ = url; hasPending_ = true; return; }
  launch(url, generation_);
}
void Metadata::launch(const QUrl& url, quint64 generation) {
  running_ = true;
  auto state = state_; auto reader = reader_;
  QThreadPool::globalInstance()->start([state, reader, url, generation] {
    QString result;
    try { result = reader(url); }
    catch (const std::exception& e) { result = QStringLiteral("Error: %1").arg(QString::fromUtf8(e.what())); }
    catch (...) { result = QStringLiteral("Error reading metadata."); }
    std::lock_guard lock(state->mutex);
    if (auto* owner = state->owner) {
      QMetaObject::invokeMethod(owner, [owner, generation, result] { owner->completed(generation, result); }, Qt::QueuedConnection);
    }
  });
}
void Metadata::completed(quint64 generation, const QString& result) {
  running_ = false;
  if (generation == generation_) emit statusChanged(result);
  if (hasPending_) { hasPending_ = false; launch(pending_, generation_); }
}
