#include "analysis_presentation.hpp"
#include <QtTest>

class AnalysisPresentationTest final : public QObject {
  Q_OBJECT
private slots:
  void publishesTheCompleteSharedProjection() {
    AnalysisPresentation model;
    beatglass::Snapshot snapshot;
    snapshot.state = beatglass::TrackingState::tracking;
    snapshot.bpm = 128.25f;
    snapshot.observed_through = {4, 9000};
    snapshot.analysis_delay_frames = 321;
    snapshot.tempo_history = {126.0f, 128.25f};
    snapshot.beats = {{{4, 1000}, 0.8f}, {{4, 2000}, 0.9f}, {{3, 3000}, 0.7f}};
    snapshot.chords = {{{4, 0}, {4, 2000}, {"Am", 0.7f}}, {{4, 2000}, {4, 4000}, {"F", 0.6f}}};
    model.publish(snapshot);

    QCOMPARE(model.bpm(), QStringLiteral("128.2"));
    QCOMPARE(model.beatCount(), qulonglong(2));
    QCOMPARE(model.analysisDelayFrames(), qulonglong(321));
    QCOMPARE(model.tempoHistory(), QStringList({"126.0", "128.2"}));
    QCOMPARE(model.tonalHistory(), QStringList({"Am", "F"}));
    QCOMPARE(model.beatFrames(), QVariantList({QVariant::fromValue<qulonglong>(1000), QVariant::fromValue<qulonglong>(2000)}));
  }

  void clearsBeatCountWhenEvidenceBecomesStale() {
    AnalysisPresentation model;
    beatglass::Snapshot tracking;
    tracking.state = beatglass::TrackingState::tracking;
    tracking.observed_through = {1, 5000};
    tracking.beats = {{{1, 4000}, 1.0f}};
    model.publish(tracking);
    QCOMPARE(model.beatCount(), qulonglong(1));

    beatglass::Snapshot reacquiring;
    reacquiring.state = beatglass::TrackingState::reacquiring;
    reacquiring.observed_through = {1, 5000};
    model.publish(reacquiring);
    QVERIFY(model.stale());
    QCOMPARE(model.beatCount(), qulonglong(0));
    QCOMPARE(model.beatFrames(), QVariantList{});
  }
};

QTEST_GUILESS_MAIN(AnalysisPresentationTest)
#include "analysis_presentation_test.moc"
