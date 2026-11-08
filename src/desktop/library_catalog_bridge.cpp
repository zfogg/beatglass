#include "library_catalog_bridge.hpp"

#include "beatglass/library/catalog.hpp"

#include <QVariantMap>

QVariantList LibraryCatalogBridge::browse(const QUrl& folder, const QString& search) const {
  if (!folder.isLocalFile()) return {};
  const auto result = beatglass::library::browse(folder.toLocalFile().toStdString(), search.toStdString());
  QVariantList rows;
  for (const auto& entry : result.entries) {
    QVariantMap row;
    row.insert(QStringLiteral("name"), QString::fromStdString(entry.name));
    row.insert(QStringLiteral("path"), QString::fromStdString(entry.path.string()));
    row.insert(QStringLiteral("folder"), entry.directory);
    row.insert(QStringLiteral("audio"), !entry.directory);
    rows.push_back(row);
  }
  return rows;
}
