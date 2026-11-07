#include "app_lifecycle.hpp"
#include <QApplication>
#include <QProcess>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QLocalSocket>
#include <QThread>
#include <QtTest>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#endif

class AppLifecycleTest : public QObject {
  Q_OBJECT
private slots:
  void fallbackAndRecoveryLoss() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("fallback"), directory.path());
    QCOMPARE(lifecycle.start(), AppLifecycle::Startup::primary);
    QQuickWindow controls; controls.show(); lifecycle.attach(&controls);
    QVERIFY(!lifecycle.recoveryAvailable());
    QVERIFY(!lifecycle.hideControls()); QVERIFY(controls.isVisible());
    controls.hide(); lifecycle.recoveryAvailable_ = true;
    bool signalBeforeRestore = false;
    connect(&lifecycle, &AppLifecycle::recoveryAvailableChanged, &lifecycle, [&] { signalBeforeRestore = !lifecycle.recoveryAvailable() && !controls.isVisible(); });
    lifecycle.refreshRecovery();
    QVERIFY(signalBeforeRestore); QVERIFY(controls.isVisible());
  }
  void relaunchRestoresExistingWindow() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("relaunch"), directory.path());
    QCOMPARE(lifecycle.start(), AppLifecycle::Startup::primary);
    QQuickWindow controls; controls.show(); lifecycle.attach(&controls); controls.hide();
    QProcess child; child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--secondary"), directory.path()});
    QVERIFY(child.waitForStarted(5000));
    QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 10000);
    QCOMPARE(child.exitStatus(), QProcess::NormalExit); QCOMPARE(child.exitCode(), 0);
    QVERIFY(controls.isVisible());
  }
  void relaunchWaitsForPrimaryListener() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("relaunch"), directory.path());
    QVERIFY(lifecycle.lock_.tryLock(0)); lifecycle.primary_ = true;
    QQuickWindow controls; controls.show(); lifecycle.attach(&controls); controls.hide();
    QTimer::singleShot(300, &lifecycle, [&] { QVERIFY(lifecycle.server_.listen(lifecycle.serverName_)); });
    QProcess child; child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--secondary"), directory.path()});
    QVERIFY(child.waitForStarted(5000)); QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 10000);
    QCOMPARE(child.exitStatus(), QProcess::NormalExit); QCOMPARE(child.exitCode(), 0); QVERIFY(controls.isVisible());
  }
  void fragmentedAcknowledgement() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto name = QStringLiteral("beatglass-fragment-") + QString::number(QCoreApplication::applicationPid());
    QLocalServer::removeServer(name); QLocalServer server; QVERIFY(server.listen(name));
    QProcess child; child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--fragment"), name});
    QVERIFY(child.waitForStarted(5000)); QVERIFY(server.waitForNewConnection(5000));
    std::unique_ptr<QLocalSocket> socket(server.nextPendingConnection()); QVERIFY(socket);
    QVERIFY(AppLifecycle::readAcknowledgement(*socket, 5000));
    QVERIFY(child.waitForFinished(5000)); QCOMPARE(child.exitCode(), 0);
  }
  void relaunchTakesOverAfterPrimaryExits() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle old(QStringLiteral("relaunch"), directory.path());
    QCOMPARE(old.start(), AppLifecycle::Startup::primary); old.server_.close();
    QTimer::singleShot(300, &old, [&] { old.lock_.unlock(); });
    QProcess child; child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--promote"), directory.path()});
    QVERIFY(child.waitForStarted(5000)); QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 10000);
    QCOMPARE(child.exitStatus(), QProcess::NormalExit); QCOMPARE(child.exitCode(), 0);
  }
  void teardownClosesPendingHandoff() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    auto lifecycle = std::make_unique<AppLifecycle>(QStringLiteral("pending"), directory.path());
    QCOMPARE(lifecycle->start(), AppLifecycle::Startup::primary);
    QLocalSocket peer; peer.connectToServer(lifecycle->serverName_); QVERIFY(peer.waitForConnected(5000));
    peer.write("restore\n"); peer.flush();
    QTRY_VERIFY_WITH_TIMEOUT(!lifecycle->clients_.isEmpty() && (*lifecycle->clients_.begin())->property("waitingRestore").toBool(), 5000);
    QLocalSocket silent; silent.connectToServer(lifecycle->serverName_); QVERIFY(silent.waitForConnected(5000));
    QTRY_COMPARE_WITH_TIMEOUT(lifecycle->clients_.size(), 2, 5000);
    QCOMPARE(peer.bytesAvailable(), 0);
    lifecycle.reset(); QTRY_COMPARE_WITH_TIMEOUT(peer.state(), QLocalSocket::UnconnectedState, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(silent.state(), QLocalSocket::UnconnectedState, 5000);
  }
  void startupAcknowledgesOnlyAfterAttachment() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("startup"), directory.path());
    QCOMPARE(lifecycle.start(), AppLifecycle::Startup::primary);
    QLocalSocket peer; peer.connectToServer(lifecycle.serverName_); QVERIFY(peer.waitForConnected(5000));
    peer.write("restore\n"); peer.flush();
    QTRY_VERIFY_WITH_TIMEOUT(!lifecycle.clients_.isEmpty() && (*lifecycle.clients_.begin())->property("waitingRestore").toBool(), 5000);
    QCOMPARE(peer.bytesAvailable(), 0);
    QQuickWindow controls; lifecycle.attach(&controls);
    QTRY_VERIFY_WITH_TIMEOUT(peer.bytesAvailable() > 0, 5000); QCOMPARE(peer.readAll(), QByteArrayLiteral("restored\n"));
    QVERIFY(controls.isVisible());
  }
  void silentAndPartialClientsExpire() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("limited"), directory.path());
    QCOMPARE(lifecycle.start(), AppLifecycle::Startup::primary);
    std::vector<std::unique_ptr<QLocalSocket>> peers;
    for (int i = 0; i < 10; ++i) {
      auto peer = std::make_unique<QLocalSocket>(); peer->connectToServer(lifecycle.serverName_);
      if (peer->waitForConnected(500)) { if (i % 2) { peer->write("res"); peer->flush(); } peers.push_back(std::move(peer)); }
      QCoreApplication::processEvents();
    }
    QVERIFY(lifecycle.clients_.size() <= 8);
    QTRY_VERIFY_WITH_TIMEOUT(lifecycle.clients_.isEmpty(), 5000);
    QQuickWindow controls; controls.show(); lifecycle.attach(&controls); controls.hide();
    QProcess child; child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--limited"), directory.path()});
    QVERIFY(child.waitForStarted(5000)); QTRY_COMPARE_WITH_TIMEOUT(child.state(), QProcess::NotRunning, 10000);
    QCOMPARE(child.exitCode(), 0); QVERIFY(controls.isVisible());
  }
  void applicationActivationRestoresControls() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    AppLifecycle lifecycle(QStringLiteral("activation"), directory.path());
    QCOMPARE(lifecycle.start(), AppLifecycle::Startup::primary);
    QQuickWindow controls; controls.show(); lifecycle.attach(&controls); controls.hide();
    QEvent activation(QEvent::ApplicationActivate); QCoreApplication::sendEvent(qApp, &activation);
    QVERIFY(controls.isVisible());
  }
};
int main(int argc, char** argv) {
#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#ifdef _MSC_VER
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE); _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE); _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR); _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR); _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
#endif
  QApplication app(argc, argv);
  if (app.arguments().size() == 3 && (app.arguments().at(1) == QStringLiteral("--secondary") || app.arguments().at(1) == QStringLiteral("--limited"))) {
    AppLifecycle secondary(app.arguments().at(1) == QStringLiteral("--limited") ? QStringLiteral("limited") : QStringLiteral("relaunch"), app.arguments().at(2));
    return secondary.start() == AppLifecycle::Startup::restored ? 0 : 1;
  }
  if (app.arguments().size() == 3 && app.arguments().at(1) == QStringLiteral("--promote")) {
    AppLifecycle secondary(QStringLiteral("relaunch"), app.arguments().at(2));
    return secondary.start() == AppLifecycle::Startup::primary ? 0 : 1;
  }
  if (app.arguments().size() == 3 && app.arguments().at(1) == QStringLiteral("--fragment")) {
    QLocalSocket peer; peer.connectToServer(app.arguments().at(2));
    if (!peer.waitForConnected(5000)) return 1;
    peer.write("rest"); peer.flush(); QThread::msleep(100); peer.write("ored\n"); peer.flush();
    if (peer.bytesToWrite() && !peer.waitForBytesWritten(5000)) return 1;
    return 0;
  }
  AppLifecycleTest test; return QTest::qExec(&test, argc, argv);
}
#include "app_lifecycle_test.moc"
