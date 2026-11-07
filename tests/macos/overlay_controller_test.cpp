#include "macos_overlay.hpp"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtTest>
#include <memory>
#include <limits>
using namespace beatglass::macos;

class FakeNative final : public OverlayNative {
public:
  bool fail{}, mode{};
  int calls{};
  bool apply(QQuickWindow*, bool clickThrough) override { ++calls; if (fail) return false; mode = clickThrough; return true; }
};
class FakeMetadata final : public QObject {
  Q_OBJECT
public:
  Q_INVOKABLE void inspect(const QUrl&) {}
signals:
  void statusChanged(const QString& text);
};
class FakeLive final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString status READ status CONSTANT)
  Q_PROPERTY(QString tempo READ unknown CONSTANT)
  Q_PROPERTY(QString key READ unknown CONSTANT)
  Q_PROPERTY(QString chord READ unknown CONSTANT)
public:
  QString status() const { return QStringLiteral("Stopped"); }
  QString unknown() const { return QStringLiteral("—"); }
};
class OverlayControlTest final : public QObject {
  Q_OBJECT
private slots:
  void escapeAndOpacity() {
    QQuickWindow overlay; QWindow control;
    overlay.setFlag(Qt::WindowTransparentForInput, true);
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
    QVERIFY(!controller.setClickThrough(true));
    control.show(); QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true)); QVERIFY(backend->mode); QVERIFY(controller.clickThrough());
    control.hide(); QVERIFY(!controller.clickThrough()); QVERIFY(!backend->mode);
    control.show(); QVERIFY(controller.setClickThrough(true));
    control.setWindowState(Qt::WindowMinimized); QTRY_VERIFY(!controller.clickThrough());
    QVERIFY(!controller.setClickThrough(true));
    controller.setOpacity(0.4); QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
    controller.setOpacity(std::numeric_limits<qreal>::quiet_NaN()); QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
  }
  void nativeFailureHidesAndDetaches() {
    QQuickWindow overlay; QWindow control; control.show();
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(controller.attach(&overlay, &control)); QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true)); backend->fail = true;
    QVERIFY(!controller.setClickThrough(false)); QVERIFY(!overlay.isVisible());
    QVERIFY(!controller.available()); QVERIFY(!controller.clickThrough());
    QVERIFY(!controller.setClickThrough(true));
  }
  void invalidWindowsAndDestroyedControl() {
    QQuickWindow overlay;
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(!controller.attach(&overlay, &overlay)); QVERIFY(!controller.available());
    auto control = std::make_unique<QWindow>(); control->show();
    QVERIFY(controller.attach(&overlay, control.get())); QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true)); control.reset();
    QVERIFY(!controller.available()); QVERIFY(!controller.clickThrough()); QVERIFY(!overlay.isVisible()); QVERIFY(!backend->mode);
  }
  void restoresGeometry() {
    QQuickWindow overlay; QWindow control;
    OverlayController controller(std::make_unique<FakeNative>());
    QVERIFY(controller.attach(&overlay, &control));
    overlay.setGeometry(-30000, -30000, 300, 200); controller.keepOnScreen();
    QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay.geometry()));
  }
  void qmlAttachesTheRealController() {
    FakeMetadata metadata; FakeLive live;
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &live);
    engine.rootContext()->setContextProperty("macOverlayController", &controller);
    engine.load(QUrl::fromLocalFile(QStringLiteral(BEATGLASS_SOURCE_DIR "/src/desktop/qml/Main.qml")));
    QVERIFY(!engine.rootObjects().isEmpty());
    QTRY_VERIFY(controller.available());
    auto* root = qobject_cast<QWindow*>(engine.rootObjects().first());
    QVERIFY(root); QTRY_VERIFY(root->isVisible());
    QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true)); QVERIFY(backend->mode);
    root->hide(); QTRY_VERIFY(!controller.clickThrough()); QVERIFY(!backend->mode);
  }
};
QTEST_MAIN(OverlayControlTest)
#include "overlay_controller_test.moc"
