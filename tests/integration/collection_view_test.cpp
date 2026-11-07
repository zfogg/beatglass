#include "collection_view.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QThread>
#include <QtTest>

class CollectionViewTest : public QObject {
  Q_OBJECT
private slots:
  void realWorkerDeliversOwnedCurrentPage() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath("library.sqlite");
    {
      beatglass::library_detail::SqliteCache db(path.toStdString());
      beatglass::library_detail::CollectionEntry a; a.path = "alpha.wav"; a.title = "Alpha"; db.put_entry(a);
      beatglass::library_detail::CollectionEntry b; b.path = "beta.wav"; b.title = "Beta"; db.put_entry(b);
    }
    CollectionView view(path);
    bool guiThread = true;
    connect(&view, &CollectionView::changed, &view, [&] { guiThread &= QThread::currentThread() == view.thread(); });
    view.refresh("alpha"); view.refresh("beta");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Showing up to 100 indexed files."), 5000);
    QCOMPARE(view.rows().size(), 1);
    QCOMPARE(view.rows().front().toMap().value("path").toString(), QStringLiteral("beta.wav"));
    QVERIFY(guiThread);
    view.refresh("no match");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("No indexed files."), 5000);
    QVERIFY(view.rows().isEmpty());
  }
  void destructionCancelsPendingDelivery() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    auto view = std::make_unique<CollectionView>(directory.filePath("empty.sqlite"));
    view->refresh(""); view.reset(); QCoreApplication::processEvents();
  }
};
QTEST_GUILESS_MAIN(CollectionViewTest)
#include "collection_view_test.moc"
