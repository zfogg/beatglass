#include "metadata.hpp"
#include <QCoreApplication>
#include <QSemaphore>
#include <QSignalSpy>
#include <QThreadPool>
#include <QTimer>
#include <stdexcept>
#include <QtTest>
#include <atomic>
#include <memory>
class MetadataTest : public QObject {
 Q_OBJECT
private slots:
 void coalescesAndIgnoresStaleResults() {
   auto entered = std::make_shared<QSemaphore>(); auto release = std::make_shared<QSemaphore>();
   auto calls = std::make_shared<std::atomic<int>>(0);
   Metadata model(nullptr, [entered, release, calls](const QUrl& url) {
     if (++*calls == 1) { entered->release(); release->acquire(); }
     return url.toString();
   });
   QSignalSpy spy(&model, &Metadata::statusChanged);
   model.inspect(QUrl("file:///first"));
   QVERIFY(entered->tryAcquire(1, 2000));
   model.inspect(QUrl("file:///second")); model.inspect(QUrl("file:///third"));
   bool eventLoopResponsive = false;
   QTimer::singleShot(0, [&] { eventLoopResponsive = true; });
   QTRY_VERIFY(eventLoopResponsive);
   QCOMPARE(spy.count(), 3); release->release();
   QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 4, 3000);
   QCOMPARE(spy.last().at(0).toString(), QString("file:///third"));
   QCOMPARE(calls->load(), 2);
 }
 void destroyedOwnerDoesNotReceiveLateResult() {
   auto entered = std::make_shared<QSemaphore>(); auto release = std::make_shared<QSemaphore>(); auto done = std::make_shared<QSemaphore>();
   auto model = std::make_unique<Metadata>(nullptr, [entered, release, done](const QUrl&) { entered->release(); release->acquire(); done->release(); return QString("late"); });
   model->inspect(QUrl("file:///slow")); QVERIFY(entered->tryAcquire(1, 2000));
   model.reset(); release->release(); QVERIFY(done->tryAcquire(1, 2000));
   QVERIFY(QThreadPool::globalInstance()->waitForDone(2000));
   QCoreApplication::processEvents();
 }
 void reportsReaderFailure() {
   Metadata model(nullptr, [](const QUrl&) -> QString { throw std::runtime_error("read failed"); });
   QSignalSpy spy(&model, &Metadata::statusChanged);
   model.inspect(QUrl("file:///broken"));
   QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 3000);
   QVERIFY(spy.last().at(0).toString().contains("read failed"));
 }
};
QTEST_GUILESS_MAIN(MetadataTest)
#include "metadata_test.moc"
