#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QString>

/** QObject bridge exposing the synchronous library catalog to QML.
 * Calls run on the GUI thread and must be limited to small, user-driven folder
 * queries; the bridge never mutates user files and returns value-owned rows.
 */
class LibraryCatalogBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString lastDiagnostic READ lastDiagnostic NOTIFY diagnosticChanged)
 public:
  using QObject::QObject;

  /** Browse one local folder, returning name/path/folder/audio role values. */
  Q_INVOKABLE QVariantList browse(const QUrl& folder, const QString& search = {});
  /** Recursively enumerate supported audio and folders under a local root. */
  Q_INVOKABLE QVariantList browseRecursive(const QUrl& folder, const QString& search = {});
  QString lastDiagnostic() const { return last_diagnostic_; }

 signals:
  void diagnosticChanged();

 private:
  QString last_diagnostic_;
};
