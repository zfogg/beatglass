#include "collection_view.hpp"
#include <QMetaObject>
#include <stdexcept>

CollectionView::CollectionView(const QString& database, QObject* parent) : QObject(parent) {
  worker_ = std::make_unique<LibraryBridge>(database.toUtf8().toStdString(), [this](auto task) {
    if (!QMetaObject::invokeMethod(this, std::move(task), Qt::QueuedConnection))
      throw std::runtime_error("GUI dispatch failed");
  });
}
CollectionView::~CollectionView() { worker_.reset(); }
void CollectionView::refresh(const QString& search) {
  auto token = requests_.beginBrowse(); rows_.clear(); status_ = QStringLiteral("Loading indexed files…"); emit changed();
  beatglass::library_detail::CollectionQuery query; query.search = search.toUtf8().toStdString(); query.limit = 100;
  worker_->entries(std::move(query), [this, token](LibraryBridge::Result result) {
    if (!result.error.empty() || result.cancelled) {
      if (!requests_.finishBrowse(token, {})) return;
      status_ = result.cancelled ? QStringLiteral("Query cancelled.") : QStringLiteral("Library error: %1").arg(QString::fromStdString(result.error));
    } else if (auto page = std::get_if<LibraryBridge::Page>(&result.value)) {
      if (!requests_.finishBrowse(token, std::move(*page))) return;
      for (const auto& entry : *requests_.page()) {
        QVariantMap row; row.insert(QStringLiteral("path"), QString::fromStdString(entry.path));
        row.insert(QStringLiteral("title"), QString::fromStdString(entry.title));
        row.insert(QStringLiteral("artist"), QString::fromStdString(entry.artist)); rows_.push_back(row);
      }
      status_ = rows_.isEmpty() ? QStringLiteral("No indexed files.") : QStringLiteral("Showing up to 100 indexed files.");
    } else {
      if (!requests_.finishBrowse(token, {})) return;
      status_ = QStringLiteral("Unexpected library response.");
    }
    emit changed();
  });
}
