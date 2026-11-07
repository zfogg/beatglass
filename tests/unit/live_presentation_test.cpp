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
    snapshot->chords.push_back({{}, {}, {"Am", 0.5f}});
    QVERIFY(model.apply({1, snapshot}));
    QCOMPARE(model.status(), QString("Tracking"));
    QCOMPARE(model.tempo(), QString("123.4"));
    QCOMPARE(model.key(), QString("C major"));
    QCOMPARE(model.chord(), QString("Am"));
    auto silent = std::make_shared<beatglass::Snapshot>(*snapshot);
    silent->state = beatglass::TrackingState::silent;
    QVERIFY(model.apply({2, silent}));
    QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
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
