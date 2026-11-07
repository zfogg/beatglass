#pragma once
#include <QObject>
#include <QUrl>
#include <functional>
#include <memory>
/// Application-private Qt metadata bridge. Call inspect on its owning Qt thread. Reads run in the global thread pool; only the latest request result is published, with at most one pending request retained. Destruction suppresses later delivery but does not cancel a running reader.
class Metadata : public QObject {
  Q_OBJECT
public:
/// Injectable reader called on a worker thread; exceptions become status text.
  using Reader = std::function<QString(const QUrl&)>;
/// Create the bridge with an optional QObject parent and worker reader.
  explicit Metadata(QObject* parent = nullptr, Reader reader = {});
  ~Metadata() override;
/// Request inspection and emit a reading status; a newer request replaces any pending request.
  Q_INVOKABLE void inspect(const QUrl& url);
signals:
/// Emit progress, result, or error text on the object thread.
  void statusChanged(const QString& status);
private:
  struct State;
  std::shared_ptr<State> state_;
  Reader reader_;
  QUrl pending_;
  quint64 generation_ = 0;
  bool running_ = false, hasPending_ = false;
  void launch(const QUrl& url, quint64 generation);
  void completed(quint64 generation, const QString& result);
};
