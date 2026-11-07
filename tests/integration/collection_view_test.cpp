#include "collection_view.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QThread>
#include <QFile>
#include <QtTest>
#include <limits>
#ifdef _WIN32
#include "windows_dj.hpp"
#include "platform/windows_default_tempo.hpp"
#endif

class CollectionViewTest : public QObject {
  Q_OBJECT
private slots:
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  void realCompositeWorkerDeliversOnlyUnverifiedEvidence() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto audio = directory.filePath("tone.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
    beatglass::SourceEpochAllocator epochs;
    CollectionView view(directory.filePath("library.sqlite"), epochs);
    bool guiThread = true;
    connect(&view, &CollectionView::changed, &view, [&] { guiThread &= QThread::currentThread() == view.thread(); });
    view.selectSource(QUrl::fromLocalFile(audio));
    QVERIFY(view.analyzeSelected());
    view.refresh("");
    QTRY_COMPARE_WITH_TIMEOUT(view.analysisStatus(), QStringLiteral("Provisional analysis complete; caching is disabled."), 30000);
    QVERIFY(guiThread); QVERIFY(view.compositeEvidence());
    QVERIFY(!view.compositeEvidence()->cache_reusable());
    QVERIFY(view.compositeEvidence()->value.epoch != 0);
    QVERIFY(view.compositeEvidence()->value.engine_version.empty());
    QVERIFY(view.compositeEvidence()->value.configuration_id.empty());
    QVERIFY(!view.compositeEvidence()->value.validated_track_key);
    QVERIFY(view.provisionalSummary().contains(QStringLiteral("Track-level key: Unknown")));
    QVERIFY(!view.compositeEvidence()->value.tonal_windows.empty());
    QVERIFY(!view.provisionalWindows().isEmpty());
    const auto row = view.provisionalWindows().front().toMap();
    const auto& actual = view.compositeEvidence()->value.tonal_windows.front();
    QCOMPARE(row.value("key").toString(), actual.rolling_key ? QString::fromStdString(actual.rolling_key->label) : QStringLiteral("Unknown"));
    QCOMPARE(row.value("keyFrom").toString(), actual.rolling_key_from ? QString::number(static_cast<qulonglong>(*actual.rolling_key_from)) : QStringLiteral("Unknown"));
    QCOMPARE(row.value("chord").toString(), actual.local_chord ? QString::fromStdString(actual.local_chord->label) : QStringLiteral("Unknown"));
    if (!actual.rolling_key || !actual.rolling_key->strength) QCOMPARE(row.value("keyStrength").toString(), QStringLiteral("Unknown"));
    if (!actual.local_chord || !actual.local_chord->strength) QCOMPARE(row.value("chordStrength").toString(), QStringLiteral("Unknown"));
    QVERIFY(view.analyzeSelected());
    QVERIFY(!view.compositeEvidence());
    view.cancelSelected();
    QTRY_VERIFY_WITH_TIMEOUT(!view.analysisActive(), 30000);
    QCOMPARE(view.analysisStatus(), QStringLiteral("Analysis finished; result hidden."));
    QVERIFY(view.provisionalWindows().isEmpty());
    QVERIFY(view.analyzeSelected());
    view.selectSource(QUrl::fromLocalFile(directory.filePath("different.aiff")));
    QTRY_VERIFY_WITH_TIMEOUT(!view.analysisActive(), 30000);
    QCOMPARE(view.analysisStatus(), QStringLiteral("Ready for provisional analysis."));
    QVERIFY(view.provisionalWindows().isEmpty());
    QVERIFY(view.provisionalSummary().contains(QStringLiteral("Track-level key: Unknown")));
    view.selectSource(QUrl());
    QVERIFY(!view.analyzeSelected());
    QVERIFY(view.provisionalWindows().isEmpty());
  }
  void completedEvidenceCannotOutliveSelectionOrCancellation() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto audio = directory.filePath("tone.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
    beatglass::SourceEpochAllocator epochs;
    CollectionView view(directory.filePath("library.sqlite"), epochs);
    for (bool cancel : {false, true}) {
      view.selectSource(QUrl::fromLocalFile(audio));
      QVERIFY(view.analyzeSelected());
      view.poll_.stop();
      QTRY_VERIFY_WITH_TIMEOUT(view.analysisTicket_->result().has_value(), 30000);
      auto ready = view.analysisTicket_->result();
      const auto* composite = std::get_if<LibraryBridge::Composite>(&ready->value);
      QVERIFY(composite);
      QCOMPARE(composite->status, beatglass::library_detail::CompositeJobStatus::complete);
      if (cancel) view.cancelSelected();
      else view.selectSource(QUrl::fromLocalFile(directory.filePath("different.aiff")));
      const auto status = view.analysisStatus();
      view.pollAnalysis();
      QVERIFY(!view.analysisActive());
      QCOMPARE(view.analysisStatus(), cancel ? QStringLiteral("Analysis finished; result hidden.") : status);
      QVERIFY(view.provisionalWindows().isEmpty());
      QVERIFY(!view.compositeEvidence());
    }
  }
  void compositeDestructionJoinsWorker() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto audio = directory.filePath("tone.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
    beatglass::SourceEpochAllocator epochs;
    auto view = std::make_unique<CollectionView>(directory.filePath("library.sqlite"), epochs);
    int notifications = 0; QObject observer;
    connect(view.get(), &CollectionView::changed, &observer, [&] { ++notifications; });
    QVERIFY(view->analyzeComposite(audio)); const auto beforeDestroy = notifications;
    view.reset(); QCoreApplication::processEvents(); QCOMPARE(notifications, beforeDestroy);
  }
#endif
  void realWorkerDeliversOwnedCurrentPage() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath("library.sqlite");
    {
      beatglass::library_detail::SqliteCache db(path.toStdString());
      beatglass::library_detail::CollectionEntry a; a.path = "alpha.wav"; a.title = "Alpha"; db.put_entry(a);
      beatglass::library_detail::CollectionEntry b; b.path = "beta.wav"; b.title = "Beta"; db.put_entry(b);
    }
    beatglass::SourceEpochAllocator epochs; CollectionView view(path, epochs);
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
    beatglass::SourceEpochAllocator epochs; auto view = std::make_unique<CollectionView>(directory.filePath("empty.sqlite"), epochs);
    view->refresh(""); view.reset(); QCoreApplication::processEvents();
  }
#ifdef _WIN32
  void liveAndOfflineShareExhaustion() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    beatglass::SourceEpochAllocator epochs(std::numeric_limits<std::uint64_t>::max() - 2);
    {
      CollectionView view(directory.filePath("shared.sqlite"), epochs);
      WindowsDj live(epochs);
      live.start(QStringLiteral("not-a-real-endpoint"));
      QVERIFY(view.analyzeIndexed(directory.filePath("missing.wav"), "integration-test-engine", "default"));
      QTRY_VERIFY_WITH_TIMEOUT(!live.active(), 5000);
      QTRY_VERIFY_WITH_TIMEOUT(view.analysisStatus() != QStringLiteral("Analysis running."), 10000);
      QVERIFY(!epochs.next());
      QVERIFY(!view.analyzeIndexed(directory.filePath("missing.wav"), "integration-test-engine", "default"));
      QCOMPARE(view.analysisStatus(), QStringLiteral("Analysis unavailable: epoch limit reached."));
      view.lookupIndexed(directory.filePath("missing.wav"), "integration-test-engine", "default");
      QCOMPARE(view.status(), QStringLiteral("Lookup unavailable: identity or epoch missing."));
      beatglass::library_detail::OfflineKey key{"hash", "integration-test-engine", "default", beatglass::OfflineMode::causal, {}};
      view.requestOfflinePage(directory.filePath("missing.wav"), key, "", 0, 1, 0, 1);
      QCOMPARE(view.status(), QStringLiteral("Offline page unavailable: identity, run, or epoch missing."));
      live.start(QStringLiteral("not-a-real-endpoint"));
      QTRY_VERIFY_WITH_TIMEOUT(!live.active(), 5000);
      QCOMPARE(live.status(), QStringLiteral("Capture could not start"));
    }
    QVERIFY(!epochs.next());
  }
  void defaultFollowerAndOfflineShareExhaustion() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    beatglass::SourceEpochAllocator epochs(std::numeric_limits<std::uint64_t>::max() - 2);
    {
      CollectionView view(directory.filePath("default.sqlite"), epochs);
      beatglass::windows::DefaultTempoFollower follower(epochs, [] {
        beatglass::windows::EndpointResult result;
        result.endpoints.push_back({"not-a-real-endpoint", "missing", true});
        return result;
      });
      QVERIFY(follower.start());
      QVERIFY(view.analyzeIndexed(directory.filePath("missing.wav"), "integration-test-engine", "default"));
      QVERIFY(follower.tick(std::chrono::steady_clock::now()));
      QVERIFY(!epochs.next());
      QTRY_VERIFY_WITH_TIMEOUT(view.analysisStatus() != QStringLiteral("Analysis running."), 10000);
      QVERIFY(!view.analyzeIndexed(directory.filePath("missing.wav"), "integration-test-engine", "default"));
      QCOMPARE(view.analysisStatus(), QStringLiteral("Analysis unavailable: epoch limit reached."));
      follower.stop();
    }
    QVERIFY(!epochs.next());
  }
#endif
  void realIndexedAnalysisAndLookupRemainGated() {
    QTemporaryDir directory; QVERIFY(directory.isValid());
    const auto path = directory.filePath("library.sqlite");
    const auto audio = directory.filePath("tone.aiff");
    QVERIFY(QFile::copy(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff"), audio));
    {
      beatglass::library_detail::SqliteCache db(path.toStdString());
      beatglass::library_detail::CollectionEntry row; row.path = audio.toUtf8().toStdString(); db.put_entry(row);
    }
    beatglass::SourceEpochAllocator epochs; CollectionView view(path, epochs);
    view.lookupIndexed(audio, "integration-test-engine", "default");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("No indexed analysis."), 10000);
    QVERIFY(view.analyzeIndexed(audio, "integration-test-engine", "default"));
    QVERIFY(!view.analyzeIndexed(audio, "integration-test-engine", "default"));
    view.refresh("");
    QTRY_COMPARE_WITH_TIMEOUT(view.analysisStatus(), QStringLiteral("Analysis committed; result remains gated."), 15000);
    view.lookupIndexed(audio, "integration-test-engine", "default");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Provisional analysis found; result remains gated."), 10000);
    QFile changed(audio); QVERIFY(changed.open(QIODevice::Append)); QCOMPARE(changed.write("x", 1), 1); changed.close();
    view.lookupIndexed(audio, "integration-test-engine", "default");
    QTRY_COMPARE_WITH_TIMEOUT(view.status(), QStringLiteral("Source changed or cannot be verified."), 10000);
    QVERIFY(view.analyzeIndexed(audio, "integration-test-engine", "default"));
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
    SourceEpochAllocator epochs; auto view = std::make_unique<CollectionView>(db, epochs);
    view->requestOfflinePage(audio, identity, "", 0, 1, 0, 1);
    QTRY_COMPARE_WITH_TIMEOUT(view->status(), QStringLiteral("Provisional offline page received; result remains gated."), 10000);
    QVERIFY(view->offlinePage()); QCOMPARE(QString::fromStdString(view->offlinePage()->run_id), QStringLiteral("run-2"));
    const auto firstEpoch = view->offlinePage()->header.epoch;
    QVERIFY(firstEpoch != 0);
    QVERIFY(!view->offlinePage()->beats.front().strength); QVERIFY(!view->offlinePage()->windows.front().rolling_key->strength);
    QCOMPARE(view->offlinePage()->header.beats.capacity(), std::size_t(0));
    QCOMPARE(view->offlinePage()->header.tonal_windows.capacity(), std::size_t(0));
    view->requestOfflinePage(audio, identity, "run-2", 1, 1, 1, 1);
    QVERIFY(!view->offlinePage());
    QTRY_COMPARE_WITH_TIMEOUT(view->status(), QStringLiteral("Provisional offline page received; result remains gated."), 10000);
    QCOMPARE(view->offlinePage()->header.epoch, firstEpoch);
    QCOMPARE(view->offlinePage()->windows.front().start_frame, std::uint64_t(100));
    QVERIFY(view->offlinePage()->windows.front().rolling_key_from == 100);
    QVERIFY(!view->offlinePage()->header.validated_track_key);
    // Queue work, then destroy and join before delivering any later result.
    int notifications = 0; QObject observer;
    connect(view.get(), &CollectionView::changed, &observer, [&] { ++notifications; });
    view->requestOfflinePage(audio, identity, "run-2", 0, 1, 0, 1);
    const auto beforeDestroy = notifications;
    view.reset(); QCoreApplication::processEvents();
    QCOMPARE(notifications, beforeDestroy);
    view = std::make_unique<CollectionView>(db, epochs);
    QVERIFY(!view->offlinePage());
    view->requestOfflinePage(audio, identity, "", 0, 1, 0, 1);
    QTRY_COMPARE_WITH_TIMEOUT(view->status(), QStringLiteral("Provisional offline page received; result remains gated."), 10000);
    QVERIFY(view->offlinePage()->header.epoch > firstEpoch);
    view->requestOfflinePage(audio, identity, "run-1", 0, 1, 0, 1);
    QCOMPARE(view->status(), QStringLiteral("Offline page unavailable: identity, run, or epoch missing."));
    QVERIFY(!view->offlinePage());
    view->requestOfflinePage(audio, identity, "", 0, 1, 0, 1);
    view->refresh("no match");
    QTRY_COMPARE_WITH_TIMEOUT(view->status(), QStringLiteral("No indexed files."), 10000);
    QVERIFY(!view->offlinePage());
    QFile changed(audio); QVERIFY(changed.open(QIODevice::Append)); QCOMPARE(changed.write("x", 1), 1); changed.close();
    view->requestOfflinePage(audio, identity, "", 0, 1, 0, 1);
    QTRY_COMPARE_WITH_TIMEOUT(view->status(), QStringLiteral("Offline source or active run changed."), 10000);
    QVERIFY(!view->offlinePage());
  }
};
QTEST_GUILESS_MAIN(CollectionViewTest)
#include "collection_view_test.moc"
