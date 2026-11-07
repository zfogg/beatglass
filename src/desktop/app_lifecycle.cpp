#include "app_lifecycle.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QLocalSocket>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QThread>
#include <algorithm>

AppLifecycle::AppLifecycle(const QString& key, const QString& lockDirectory, QObject* parent)
    : QObject(parent), serverName_(QStringLiteral("beatglass-") + QString::fromLatin1(QCryptographicHash::hash((lockDirectory + key).toUtf8(), QCryptographicHash::Sha256).toHex().left(24))),
      lock_(QDir(lockDirectory).filePath(key + QStringLiteral(".lock"))) {
  lock_.setStaleLockTime(0);
  server_.setSocketOptions(QLocalServer::UserAccessOption);
  server_.setMaxPendingConnections(8);
  connect(&server_, &QLocalServer::newConnection, this, &AppLifecycle::acceptConnections);
  recoveryTimer_.setInterval(2000);
  connect(&recoveryTimer_, &QTimer::timeout, this, &AppLifecycle::refreshRecovery);
}
AppLifecycle::~AppLifecycle() {
  qApp->removeEventFilter(this); recoveryTimer_.stop(); tray_.reset(); menu_.reset();
  auto clients = clients_;
  for (auto* socket : server_.findChildren<QLocalSocket*>()) clients.insert(socket);
  for (auto* socket : clients) { disconnect(socket, nullptr, this, nullptr); socket->abort(); delete socket; }
  clients_.clear(); server_.close();
}
bool AppLifecycle::readAcknowledgement(QLocalSocket& socket, int timeoutMs) {
  QElapsedTimer deadline; deadline.start(); QByteArray reply;
  while (!reply.endsWith('\n') && reply.size() < 9) {
    const int remaining = timeoutMs - static_cast<int>(deadline.elapsed());
    if (remaining <= 0 || (socket.bytesAvailable() == 0 && !socket.waitForReadyRead(remaining))) return false;
    reply += socket.readAll();
  }
  return reply == QByteArrayLiteral("restored\n");
}
AppLifecycle::Startup AppLifecycle::start() {
  const auto becomePrimary = [this] {
    QLocalServer::removeServer(serverName_);
    if (!server_.listen(serverName_)) { lock_.unlock(); return Startup::unavailable; }
    primary_ = true; return Startup::primary;
  };
  if (lock_.tryLock(0)) return becomePrimary();
  QElapsedTimer deadline; deadline.start(); QLocalSocket peer; peer.setReadBufferSize(10);
  while (deadline.elapsed() < 7000) {
    if (lock_.tryLock(0)) return becomePrimary();
    peer.abort(); peer.connectToServer(serverName_);
    if (peer.waitForConnected(250) && peer.write("restore\n") == 8 &&
        (!peer.bytesToWrite() || peer.waitForBytesWritten(1000))) {
      const int remaining = 7000 - static_cast<int>(deadline.elapsed());
      if (remaining > 0 && readAcknowledgement(peer, std::min(remaining, 2000))) return Startup::restored;
    }
    QThread::msleep(50);
  }
  return Startup::unavailable;
}
std::unique_ptr<QMessageBox> AppLifecycle::handoffFailureDialog() {
  auto dialog = std::make_unique<QMessageBox>(QMessageBox::Warning, tr("Beatglass could not open"),
      tr("Beatglass could not start or restore its window."), QMessageBox::Ok);
  dialog->setInformativeText(tr("Wait a moment and try again. If another Beatglass instance is not responding, close it using your system's process manager, then reopen it."));
  return dialog;
}
bool AppLifecycle::useNoninteractiveErrors(const QString& platform, bool requested) {
  return requested || platform == QStringLiteral("offscreen") || platform.startsWith(QStringLiteral("minimal"));
}
void AppLifecycle::acceptConnections() {
  while (auto* socket = server_.nextPendingConnection()) {
    if (clients_.size() >= 8) { socket->abort(); socket->deleteLater(); continue; }
    clients_.insert(socket); socket->setReadBufferSize(9);
    connect(socket, &QLocalSocket::readyRead, this, [this, socket] { consumeRequest(socket); });
    connect(socket, &QLocalSocket::disconnected, this, [this, socket] { clients_.remove(socket); socket->deleteLater(); });
    QTimer::singleShot(2000, socket, [this, socket] { clients_.remove(socket); socket->abort(); socket->deleteLater(); });
    consumeRequest(socket);
  }
}
void AppLifecycle::consumeRequest(QLocalSocket* socket) {
  if (socket->property("waitingRestore").toBool()) { if (socket->bytesAvailable()) socket->abort(); return; }
  auto request = socket->property("request").toByteArray(); request += socket->readAll();
  if (request.size() > 8) { socket->disconnectFromServer(); return; }
  if (!request.endsWith('\n')) { socket->setProperty("request", request); return; }
  if (request == QByteArrayLiteral("restore\n")) {
    if (!controls_) { socket->setProperty("waitingRestore", true); return; }
    acknowledgeRestore(socket); return;
  }
  socket->disconnectFromServer();
}
void AppLifecycle::acknowledgeRestore(QLocalSocket* socket) {
  showControls();
  if (controls_ && controls_->isVisible()) { socket->write("restored\n"); socket->flush(); socket->disconnectFromServer(); }
  else socket->abort();
}
void AppLifecycle::attach(QQuickWindow* controls) {
  controls_ = controls;
  qApp->installEventFilter(this);
  if (restorePending_) { restorePending_ = false; showControls(); }
  const auto clients = clients_;
  for (auto* socket : clients) if (socket->property("waitingRestore").toBool()) acknowledgeRestore(socket);
  if (!primary_ || !controls_) return;
  menu_ = std::make_unique<QMenu>();
  connect(menu_->addAction(tr("Show controls")), &QAction::triggered, this, &AppLifecycle::showControls);
  connect(menu_->addAction(tr("Unlock and edit overlay")), &QAction::triggered, this, [this] { emit editOverlayRequested(); showControls(); });
  connect(menu_->addAction(tr("Hide overlay")), &QAction::triggered, this, [this] { emit hideOverlayRequested(); showControls(); });
  menu_->addSeparator();
  connect(menu_->addAction(tr("Quit Beatglass")), &QAction::triggered, qApp, &QCoreApplication::quit);
  QPixmap image(32, 32); image.fill(Qt::transparent); QPainter painter(&image);
  painter.setPen(Qt::NoPen); painter.setBrush(QColor(QStringLiteral("#67d5c0")));
  painter.drawRoundedRect(QRectF(4, 13, 5, 12), 2, 2); painter.drawRoundedRect(QRectF(13, 6, 5, 19), 2, 2); painter.drawRoundedRect(QRectF(22, 10, 5, 15), 2, 2); painter.end();
  tray_ = std::make_unique<QSystemTrayIcon>(QIcon(image)); tray_->setToolTip(tr("Beatglass")); tray_->setContextMenu(menu_.get());
  connect(tray_.get(), &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) { if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) showControls(); });
  refreshRecovery(); recoveryTimer_.start();
}
void AppLifecycle::refreshRecovery() {
  const bool available = tray_ && QSystemTrayIcon::isSystemTrayAvailable();
  if (available && !tray_->isVisible()) tray_->show();
  // Qt reports tray availability, not whether the user can reach the item.
  // Keep the controls discoverable until a platform supplies a verified route.
  const bool next = false;
  if (next == recoveryAvailable_) return;
  recoveryAvailable_ = next; emit recoveryAvailableChanged();
  if (!next && controls_ && !controls_->isVisible()) showControls();
}
bool AppLifecycle::hideControls() {
  refreshRecovery();
  if (!recoveryAvailable_ || !controls_) return false;
  controls_->hide(); return true;
}
void AppLifecycle::showControls() {
  if (!controls_) { restorePending_ = true; return; }
  controls_->showNormal(); controls_->raise(); controls_->requestActivate();
}
bool AppLifecycle::eventFilter(QObject* watched, QEvent* event) {
  if (watched == qApp && event->type() == QEvent::ApplicationActivate && controls_ && !controls_->isVisible()) showControls();
  return QObject::eventFilter(watched, event);
}
