#include "metadata.hpp"
#include "live_presentation.hpp"
#include "playback_guard.hpp"
#include "windows_dj.hpp"
#include "windows_overlay_bridge.hpp"
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QScreen>
#include <QTemporaryDir>
#include <QtTest>

class CombinedUiTest : public QObject {
  Q_OBJECT
private slots:
  void closingWhileSelectedOutputAndClickThroughAreActive() {
    beatglass::SourceEpochAllocator epochs;
    Metadata metadata;
    LivePresentation presentation;
    PlaybackGuard guard;
    WindowsDj dj(epochs);
    QPointer<QQuickWindow> rootPointer;
    QPointer<QQuickWindow> overlayPointer;
    {
      QQmlApplicationEngine engine;
      engine.rootContext()->setContextProperty("metadata", &metadata);
      engine.rootContext()->setContextProperty("livePresentation", &presentation);
      engine.rootContext()->setContextProperty("playbackGuard", &guard);
      engine.rootContext()->setContextProperty("windowsDj", &dj);
      engine.load(QUrl("qrc:/Main.qml"));
      QVERIFY(!engine.rootObjects().isEmpty());
      auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
      QVERIFY(root);
      auto* overlay = root->findChild<QQuickWindow*>("djOverlay");
      QVERIFY(overlay);
      rootPointer = root; overlayPointer = overlay;
      WindowsOverlay bridge(overlay, root);
      QVERIFY(root->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&bridge))));
      overlay->show();
      QVERIFY(bridge.setClickThrough(true));
      dj.start(QStringLiteral("not-a-real-endpoint"));
      QVERIFY(dj.active());
      root->close();
      QTRY_VERIFY(!bridge.clickThrough());
    }
    QVERIFY(rootPointer.isNull());
    QVERIFY(overlayPointer.isNull());
    dj.stop();
    QVERIFY(!dj.active());
  }
  void overlayControlsPersistWithSelectedDjBridge() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("overlay.ini");
    beatglass::SourceEpochAllocator epochs;
    Metadata metadata;
    LivePresentation presentation;
    PlaybackGuard guard;
    WindowsDj dj(epochs);
    QPoint savedPosition;
    for (int run = 0; run != 2; ++run) {
      QSettings settings(path, QSettings::IniFormat);
      QQmlApplicationEngine engine;
      engine.rootContext()->setContextProperty("metadata", &metadata);
      engine.rootContext()->setContextProperty("livePresentation", &presentation);
      engine.rootContext()->setContextProperty("playbackGuard", &guard);
      engine.rootContext()->setContextProperty("windowsDj", &dj);
      engine.load(QUrl("qrc:/Main.qml"));
      QVERIFY(!engine.rootObjects().isEmpty());
      auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
      QVERIFY(root);
      auto* overlay = root->findChild<QQuickWindow*>("djOverlay");
      auto* slider = root->findChild<QObject*>("overlayOpacity");
      auto* show = root->findChild<QObject*>("showOverlay");
      auto* lock = root->findChild<QObject*>("lockOverlay");
      auto* recover = root->findChild<QObject*>("recoverOverlay");
      QVERIFY(overlay); QVERIFY(slider); QVERIFY(show); QVERIFY(lock); QVERIFY(recover);
      WindowsOverlay bridge(overlay, root, nullptr, &settings);
      QVERIFY(root->setProperty("overlayBridge", QVariant::fromValue(static_cast<QObject*>(&bridge))));
      if (run == 0) {
        QVERIFY(QMetaObject::invokeMethod(show, "clicked"));
        QTRY_VERIFY(overlay->isVisible());
        QVERIFY(QMetaObject::invokeMethod(lock, "clicked"));
        QVERIFY(bridge.clickThrough());
        QVERIFY(slider->setProperty("value", 0.4));
        QVERIFY(QMetaObject::invokeMethod(slider, "moved"));
        QVERIFY(qAbs(bridge.opacity() - 0.4) < 0.02);
        overlay->setPosition(-30000, -30000);
        QVERIFY(QMetaObject::invokeMethod(recover, "clicked"));
        QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay->geometry()));
        savedPosition = overlay->position();
        QVERIFY(QMetaObject::invokeMethod(show, "clicked"));
        QVERIFY(!bridge.clickThrough());
        QTRY_VERIFY(!overlay->isVisible());
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
      } else {
        QCOMPARE(overlay->position(), savedPosition);
        QVERIFY(qAbs(slider->property("value").toReal() - 0.4) < 0.02);
        QVERIFY(!overlay->isVisible());
        QVERIFY(!bridge.clickThrough());
      }
    }
  }
};
QTEST_MAIN(CombinedUiTest)
#include "windows_combined_ui_test.moc"
