#include "live_presentation.hpp"
#include <QSignalSpy>
#include <QtTest>
#include <memory>
class LivePresentationTest : public QObject {
  Q_OBJECT
private slots:
  void clearRejectsDelayedValues() {
    LivePresentation model;
    QSignalSpy spy(&model, &LivePresentation::changed);
    auto snapshot = std::make_shared<beatglass::Snapshot>();
    snapshot->state = beatglass::TrackingState::tracking;
    snapshot->bpm = 123.4f;
    snapshot->key_candidates.push_back({"C major", 0.4f});
    snapshot->observed_through = {1, 2};
    snapshot->analysis_delay_frames = 17;
    snapshot->confidence = 0.75f;
    snapshot->tempo_history = {120.0f, 123.4f};
    snapshot->beats.push_back({{1, 1}, 0.8f});
    snapshot->chords.push_back({{1, 0}, {1, 1}, {"Am", 0.5f}});
    QVERIFY(model.apply({1, snapshot}));
    QCOMPARE(model.status(), QString("Tracking"));
    QCOMPARE(model.tempo(), QString("123.4"));
    QCOMPARE(model.key(), QString("C major"));
    QCOMPARE(model.chord(), QString("Am"));
    QCOMPARE(model.beatCount(), qulonglong(1));
    QCOMPARE(model.observedFrames(), qulonglong(2));
    QCOMPARE(model.analysisDelayFrames(), qulonglong(17));
    QCOMPARE(model.confidence(), 0.75);
    QCOMPARE(model.tempoHistory(), QStringList({"120.0", "123.4"}));
    QCOMPARE(model.tonalHistory(), QStringList({"Am"}));
    auto silent = std::make_shared<beatglass::Snapshot>(*snapshot);
    silent->state = beatglass::TrackingState::silent;
    QVERIFY(model.apply({2, silent}));
    QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
    QVERIFY(model.tonalHistory().isEmpty());
    QVERIFY(model.apply({3, {}}));
    QVERIFY(!model.apply({2, snapshot}));
    QCOMPARE(spy.count(), 3);
    QCOMPARE(model.status(), QString("Stopped"));
    QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
    QCOMPARE(model.key(), QStringLiteral("\u2014"));
    QCOMPARE(model.chord(), QStringLiteral("\u2014"));
  }
  void missingValuesRemainUnknown() {
    LivePresentation model;
    auto snapshot = std::make_shared<beatglass::Snapshot>();
    snapshot->state = beatglass::TrackingState::acquiring;
    snapshot->bpm = -1;
    QVERIFY(model.apply({0, snapshot}));
    QCOMPARE(model.status(), QString("Acquiring"));
    QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
  }
};
QTEST_GUILESS_MAIN(LivePresentationTest)
#include "live_presentation_test.moc"
