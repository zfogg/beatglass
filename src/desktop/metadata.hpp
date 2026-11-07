#pragma once
#include <QObject>
#include <QUrl>
#include <functional>
#include <memory>
class Metadata : public QObject {
  Q_OBJECT
public:
  using Reader = std::function<QString(const QUrl&)>;
  explicit Metadata(QObject* parent = nullptr, Reader reader = {});
  ~Metadata() override;
  Q_INVOKABLE void inspect(const QUrl& url);
signals:
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
