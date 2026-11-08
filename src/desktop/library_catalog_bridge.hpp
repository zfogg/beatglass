#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

/** QObject bridge exposing the synchronous library catalog to QML.
 * Calls run on the GUI thread and must be limited to small, user-driven folder
 * queries; the bridge never mutates user files and returns value-owned rows.
 */
class LibraryCatalogBridge final : public QObject {
  Q_OBJECT
 public:
  using QObject::QObject;

  /** Browse one local folder, returning name/path/folder/audio role values. */
  Q_INVOKABLE QVariantList browse(const QUrl& folder, const QString& search = {}) const;
};
