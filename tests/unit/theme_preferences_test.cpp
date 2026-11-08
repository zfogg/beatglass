#include "theme_preferences.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QtTest>

class ThemePreferencesTest final : public QObject {
  Q_OBJECT
private slots:
  void persistsAndClamps() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("prefs.ini"));
    {
      auto settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
      ThemePreferences prefs(nullptr, std::move(settings));
      QCOMPARE(prefs.themeIndex(), 0);
      prefs.setThemeIndex(99);
      QCOMPARE(prefs.themeIndex(), 5);
      prefs.setReduceMotion(true);
      QVERIFY(prefs.reduceMotion());
    }
    {
      auto settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
      ThemePreferences prefs(nullptr, std::move(settings));
      QCOMPARE(prefs.themeIndex(), 5);
      prefs.setThemeIndex(-2);
      QCOMPARE(prefs.themeIndex(), 0);
      QVERIFY(prefs.reduceMotion());
      prefs.reset();
      QCOMPARE(prefs.themeIndex(), 0);
      QVERIFY(!prefs.reduceMotion());
    }
    auto settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    ThemePreferences prefs(nullptr, std::move(settings));
    QCOMPARE(prefs.themeIndex(), 0);
    QVERIFY(!prefs.reduceMotion());
  }

  void resetRepairsStoredValuesWhenAlreadyDefault() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("prefs.ini"));
    {
      QSettings settings(path, QSettings::IniFormat);
      settings.setValue(QStringLiteral("desktop/themeIndex"), QStringLiteral("not-a-theme"));
      settings.setValue(QStringLiteral("desktop/reduceMotion"), QStringLiteral("not-a-bool"));
      settings.sync();
    }
    auto settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    ThemePreferences prefs(nullptr, std::move(settings));
    prefs.reset();
    QSettings repaired(path, QSettings::IniFormat);
    QCOMPARE(repaired.value(QStringLiteral("desktop/themeIndex")).toInt(), 0);
    QVERIFY(!repaired.value(QStringLiteral("desktop/reduceMotion")).toBool());
  }
};

QTEST_MAIN(ThemePreferencesTest)
#include "theme_preferences_test.moc"
