#include "macos_overlay.hpp"
#include <QGuiApplication>
#include <QCloseEvent>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
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
  void reset() noexcept override { mode = false; }
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
    QVERIFY(controller.setOpacity(0.4)); QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
    QVERIFY(!controller.setOpacity(std::numeric_limits<qreal>::quiet_NaN())); QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
    QVERIFY(controller.setOpacity(0.0)); QVERIFY(qAbs(overlay.opacity() - 0.2) < 0.02);
  }
  void controlCloseRevokesClickThrough() {
    QQuickWindow overlay; QWindow control;
    control.show();
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true));
    QCloseEvent close;
    QCoreApplication::sendEvent(&control, &close);
    QVERIFY(!controller.clickThrough());
    QVERIFY(!backend->mode);
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
  void attachFailureDoesNotExposeAnOverlay() {
    QQuickWindow overlay; QWindow control;
    auto native = std::make_unique<FakeNative>(); native->fail = true;
    OverlayController controller(std::move(native));
    QVERIFY(!controller.attach(&overlay, &control));
    QVERIFY(!controller.available()); QVERIFY(!overlay.isVisible());
    QVERIFY(!controller.showOverlay());
  }
  void hiddenControlCannotRevealOverlay() {
    QQuickWindow overlay; QWindow control;
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(!controller.showOverlay());
    QVERIFY(!overlay.isVisible());
    QVERIFY(!controller.clickThrough());
    QVERIFY(!backend->mode);
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
  void destroyedOverlayRevokesNativeMode() {
    auto overlay = std::make_unique<QQuickWindow>();
    QWindow control; control.show();
    auto native = std::make_unique<FakeNative>(); auto* backend = native.get();
    OverlayController controller(std::move(native));
    QVERIFY(controller.attach(overlay.get(), &control));
    QVERIFY(controller.showOverlay());
    QVERIFY(controller.setClickThrough(true));
    overlay.reset();
    QCoreApplication::processEvents();
    QVERIFY(!controller.available());
    QVERIFY(!controller.clickThrough());
    QVERIFY(!backend->mode);
  }
  void restoresGeometry() {
    QQuickWindow overlay; QWindow control;
    OverlayController controller(std::make_unique<FakeNative>());
    QVERIFY(controller.attach(&overlay, &control));
    overlay.setGeometry(-30000, -30000, 300, 200); controller.keepOnScreen();
    QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay.geometry()));
    QVERIFY(!controller.moveToScreen(-1));
    QVERIFY(!controller.moveToScreen(controller.screenNames().size()));
    QVERIFY(!controller.resizeOverlay(0, 200));
    QVERIFY(controller.resizeOverlay(250, 120));
    QVERIFY(controller.moveToScreen(0));
    QVERIFY(QGuiApplication::screens().at(0)->availableGeometry().contains(overlay.geometry()));
  }
  void displaySelectionTracksOverlayAndStartsAtControls() {
    QQuickWindow overlay; QWindow control;
    auto* screen = QGuiApplication::primaryScreen(); QVERIFY(screen);
    control.setGeometry(QRect(screen->availableGeometry().center(), QSize(100, 100)));
    OverlayController controller(std::make_unique<FakeNative>());
    QCOMPARE(controller.screenIndex(), -1);
    QSignalSpy changes(&controller, &OverlayController::screensChanged);
    QVERIFY(controller.attach(&overlay, &control));
    QCOMPARE(controller.screenIndex(), QGuiApplication::screens().indexOf(screen));
    QVERIFY(screen->availableGeometry().contains(overlay.geometry()));
    QVERIFY(changes.count() > 0);
    QVERIFY(controller.moveToScreen(controller.screenIndex()));
    QCOMPARE(controller.screenIndex(), QGuiApplication::screens().indexOf(screen));
  }
  void closingControlHidesOverlay() {
    QQuickWindow overlay; QWindow control; control.show();
    OverlayController controller(std::make_unique<FakeNative>());
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(controller.showOverlay());
    QVERIFY(overlay.isVisible());
    QVERIFY(control.close());
    QTRY_VERIFY(!overlay.isVisible());
    QVERIFY(!controller.clickThrough());
  }
  void screenGeometryNotificationsCoalesceAndRecoverOverlay() {
    QQuickWindow overlay; QWindow control;
    auto* screen = QGuiApplication::primaryScreen(); QVERIFY(screen);
    OverlayController controller(std::make_unique<FakeNative>());
    QVERIFY(controller.attach(&overlay, &control));
    overlay.setGeometry(-30000, -30000, 250, 120);
    QSignalSpy changes(&controller, &OverlayController::screensChanged);
    const auto bounds = screen->availableGeometry();
    QVERIFY(QMetaObject::invokeMethod(screen, "geometryChanged", Qt::DirectConnection, Q_ARG(QRect, screen->geometry())));
    QVERIFY(QMetaObject::invokeMethod(screen, "availableGeometryChanged", Qt::DirectConnection, Q_ARG(QRect, bounds)));
    QCOMPARE(changes.count(), 0);
    QTRY_COMPARE(changes.count(), 1);
    QVERIFY(bounds.contains(overlay.geometry()));
    QCOMPARE(controller.screenIndex(), QGuiApplication::screens().indexOf(screen));
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
  void restoresGeometryAndOpacityWithoutRestoringClickThrough() {
    QTemporaryDir dir; QVERIFY(dir.isValid());
    const auto path = dir.filePath(QStringLiteral("overlay.ini"));
    QRect saved;
    {
      QQuickWindow overlay; QWindow control; control.show();
      OverlayController controller(std::make_unique<FakeNative>(), nullptr, std::make_unique<QSettings>(path, QSettings::IniFormat));
      QVERIFY(controller.attach(&overlay, &control)); QVERIFY(controller.showOverlay());
      const auto bounds = QGuiApplication::primaryScreen()->availableGeometry();
      overlay.setGeometry(QRect(bounds.topLeft() + QPoint(10, 10), QSize(260, 120)));
      QVERIFY(controller.moveToScreen(0));
      controller.setOpacity(0.55); QVERIFY(controller.setClickThrough(true));
      saved = overlay.geometry();
    }
    {
      QQuickWindow overlay; QWindow control; control.show();
      OverlayController controller(std::make_unique<FakeNative>(), nullptr, std::make_unique<QSettings>(path, QSettings::IniFormat));
      QVERIFY(controller.attach(&overlay, &control));
      QCOMPARE(overlay.geometry(), saved);
      QSettings persisted(path, QSettings::IniFormat);
      QCOMPARE(persisted.value(QStringLiteral("overlay/displayName")).toString(), QGuiApplication::screens().at(0)->name());
      QVERIFY(qAbs(controller.opacity() - 0.55) < 0.02);
      QVERIFY(!controller.clickThrough());
    }
  }
  void unsafeSavedSettingsAreRecovered() {
    QTemporaryDir dir; QVERIFY(dir.isValid());
    const auto path = dir.filePath(QStringLiteral("overlay.ini"));
    {
      QSettings seed(path, QSettings::IniFormat);
      seed.setValue(QStringLiteral("overlay/geometry"), QRect(-30000, -30000, 280, 120));
      seed.setValue(QStringLiteral("overlay/opacity"), 0.0);
    }
    QQuickWindow overlay; QWindow control;
    OverlayController controller(std::make_unique<FakeNative>(), nullptr, std::make_unique<QSettings>(path, QSettings::IniFormat));
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay.geometry()));
    QVERIFY(qAbs(controller.opacity() - 0.2) < 0.02);
    QVERIFY(!controller.clickThrough());
  }
  void pendingSettingsSurviveOverlayDestruction() {
    QTemporaryDir dir; QVERIFY(dir.isValid());
    const auto path = dir.filePath(QStringLiteral("overlay.ini"));
    {
      auto overlay = std::make_unique<QQuickWindow>(); QWindow control;
      OverlayController controller(std::make_unique<FakeNative>(), nullptr, std::make_unique<QSettings>(path, QSettings::IniFormat));
      QVERIFY(controller.attach(overlay.get(), &control));
      controller.setOpacity(0.65);
      overlay.reset();
      QVERIFY(!controller.available());
    }
    QQuickWindow overlay; QWindow control;
    OverlayController controller(std::make_unique<FakeNative>(), nullptr, std::make_unique<QSettings>(path, QSettings::IniFormat));
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(qAbs(controller.opacity() - 0.65) < 0.02);
  }
};
QTEST_MAIN(OverlayControlTest)
#include "overlay_controller_test.moc"
