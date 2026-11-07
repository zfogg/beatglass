#include "collection_view.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QThread>
#include <QFile>
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
  void realIndexedAnalysisAndLookupRemainGated() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath("library.sqlite");
    const auto audio = directory.filePath("tone.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
    {
      beatglass::library_detail::SqliteCache db(path.toStdString());
      beatglass::library_detail::CollectionEntry row; row.path = audio.toUtf8().toStdString(); db.put_entry(row);
    }
    CollectionView view(path);
    view.lookupIndexed(audio, "integration-test-engine", "default", 1);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("No indexed analysis."), 10000);
    QVERIFY(view.analyzeIndexed(audio, "integration-test-engine", "default", 2));
    QVERIFY(!view.analyzeIndexed(audio, "integration-test-engine", "default", 3));
    view.refresh("");
    QTRY_COMPARE_WITH_TIMEOUT(view.analysisStatus(), QStringLiteral("Analysis committed; result remains gated."), 15000);
    view.lookupIndexed(audio, "integration-test-engine", "default", 4);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Provisional analysis found; result remains gated."), 10000);
    QFile changed(audio); QVERIFY(changed.open(QIODevice::Append)); QCOMPARE(changed.write("x", 1), 1); changed.close();
    view.lookupIndexed(audio, "integration-test-engine", "default", 5);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Source changed or cannot be verified."), 10000);
    QVERIFY(view.analyzeIndexed(audio, "integration-test-engine", "default", 6));
    view.cancelAnalysis();
    QCOMPARE(view.analysisStatus(), QStringLiteral("Cancellation requested; awaiting terminal outcome."));
    QTRY_VERIFY_WITH_TIMEOUT(view.analysisStatus() != QStringLiteral("Cancellation requested; awaiting terminal outcome."), 15000);
    QVERIFY(view.analysisStatus() == QStringLiteral("Analysis cancelled.") ||
            view.analysisStatus() == QStringLiteral("Analysis committed; result remains gated.") ||
            view.analysisStatus() == QStringLiteral("Existing analysis found; result remains gated."));
  }
};
QTEST_GUILESS_MAIN(CollectionViewTest)
#include "collection_view_test.moc"
