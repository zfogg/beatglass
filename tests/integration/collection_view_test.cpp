#include "collection_view.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
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
  void ownedOfflinePagesKeepRunAndEvidence() {
    using namespace beatglass;
    using namespace beatglass::library_detail;
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto db = directory.filePath("archive.sqlite");
    const auto audio = directory.filePath("offline.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
#ifdef _WIN32
    const auto native = std::filesystem::path(audio.toStdWString());
#else
    const auto native = std::filesystem::u8path(audio.toUtf8().toStdString());
#endif
    auto snapshot = SnapshotOpen::create(native); QVERIFY(snapshot.status == SnapshotStatus::ok);
    FileVersion file{native, snapshot.value->size(), *snapshot.value->modified_unix_ns(), snapshot.value->digest()};
    OfflineKey identity{file.content_hash,"test-engine","test-config",OfflineMode::causal,{{"tonal","test"}}};
    OfflineAnalysis value; value.epoch=10; value.sample_rate=48000; value.observed_through=1000; value.analyzed_through=900;
    value.engine_version=identity.engine_version; value.configuration_id=identity.configuration_id; value.algorithms=identity.algorithms;
    value.beats={{100,200,{}},{300,400,0.5f}};
    value.tonal_windows={{0,500,0,OfflineEstimate{"C", {}},{}},{100,700,100,OfflineEstimate{"G", 0.3f},{}}};
    struct Verifier : FileVersionVerifier {
      FileSnapshot& snapshot; explicit Verifier(FileSnapshot& s) : snapshot(s) {}
      FileVerificationResult verify(std::stop_token stop) override {
        return {snapshot.verify(stop)==SnapshotStatus::ok ? FileVerificationOutcome::match : FileVerificationOutcome::changed,{}};
      }
    } verifier(*snapshot.value);
    {
      TempoAnalysisStore store(db.toStdString()); CollectionEntry entry; entry.path=audio.toUtf8().toStdString(); store.put_entry(entry);
      OfflineArchive archive(store.offline_cache());
      QVERIFY(archive.publish(file,identity,value,"run-1",entry.path,verifier,{}).outcome==PublishOutcome::committed);
      QVERIFY(archive.publish(file,identity,value,"run-2",entry.path,verifier,{}).outcome==PublishOutcome::committed);
    }
    snapshot.value.reset();
    CollectionView view(db);
    view.requestOfflinePage(audio, identity, "", 0, 1, 0, 1, 11);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Provisional offline page received; result remains gated."), 10000);
    QVERIFY(view.offlinePage()); QCOMPARE(QString::fromStdString(view.offlinePage()->run_id), QStringLiteral("run-2"));
    QVERIFY(!view.offlinePage()->beats.front().strength); QVERIFY(!view.offlinePage()->windows.front().rolling_key->strength);
    view.requestOfflinePage(audio, identity, "run-2", 1, 1, 1, 1, 11);
    QVERIFY(!view.offlinePage());
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Provisional offline page received; result remains gated."), 10000);
    QCOMPARE(view.offlinePage()->windows.front().start_frame, std::uint64_t(100));
    QVERIFY(view.offlinePage()->windows.front().rolling_key_from == 100);
    QVERIFY(!view.offlinePage()->header.validated_track_key);
    view.requestOfflinePage(audio, identity, "run-1", 0, 1, 0, 1, 11);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Offline source or active run changed."), 10000);
    QVERIFY(!view.offlinePage());
    view.requestOfflinePage(audio, identity, "run-2", 0, 1, 0, 1, 11);
    view.refresh("no match");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("No indexed files."), 10000);
    QVERIFY(!view.offlinePage());
    QFile changed(audio); QVERIFY(changed.open(QIODevice::Append)); QCOMPARE(changed.write("x", 1), 1); changed.close();
    view.requestOfflinePage(audio, identity, "run-2", 0, 1, 0, 1, 11);
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Offline source or active run changed."), 10000);
    QVERIFY(!view.offlinePage());
  }
};
QTEST_GUILESS_MAIN(CollectionViewTest)
#include "collection_view_test.moc"
