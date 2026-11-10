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
    snapshot.tempo_history = {126.0f, 128.25f};
    snapshot.beats = {{{4, 1000}, 0.8f}, {{4, 2000}, 0.9f}, {{3, 3000}, 0.7f}};
    snapshot.chords = {{{4, 0}, {4, 2000}, {"Am", 0.7f}}, {{4, 2000}, {4, 4000}, {"F", 0.6f}}};
    model.publish(snapshot);

    QCOMPARE(model.bpm(), QStringLiteral("128.2"));
    QCOMPARE(model.beatCount(), qulonglong(2));
    QCOMPARE(model.tempoHistory(), QStringList({"126.0", "128.2"}));
    QCOMPARE(model.tonalHistory(), QStringList({"Am", "F"}));
    QCOMPARE(model.beatFrames(), QVariantList({QVariant::fromValue<qulonglong>(1000), QVariant::fromValue<qulonglong>(2000)}));
  }
};

QTEST_GUILESS_MAIN(AnalysisPresentationTest)
#include "analysis_presentation_test.moc"
