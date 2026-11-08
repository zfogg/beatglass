#include "library_catalog_bridge.hpp"

#include "beatglass/library/catalog.hpp"

#include <QVariantMap>

QVariantList LibraryCatalogBridge::browse(const QUrl& folder, const QString& search) {
  const auto diagnostic = [this](QString value) {
    if (last_diagnostic_ == value) return;
    last_diagnostic_ = std::move(value);
    emit diagnosticChanged();
  };
  if (!folder.isLocalFile()) { diagnostic(QStringLiteral("The selected location is not a local folder.")); return {}; }
  const auto result = beatglass::library::browse(folder.toLocalFile().toStdString(), search.toStdString());
  diagnostic(QString::fromStdString(result.diagnostic));
  QVariantList rows;
  for (const auto& entry : result.entries) {
    QVariantMap row;
    row.insert(QStringLiteral("name"), QString::fromStdString(entry.name));
    row.insert(QStringLiteral("path"), QString::fromStdString(entry.path.string()));
    row.insert(QStringLiteral("folder"), entry.directory);
    row.insert(QStringLiteral("audio"), !entry.directory);
    row.insert(QStringLiteral("bytes"), QVariant::fromValue<qulonglong>(entry.bytes));
    rows.push_back(row);
  }
  return rows;
}
