#include "library_preferences.hpp"

LibraryPreferences::LibraryPreferences(QObject* parent)
    : QObject(parent), settings_(QSettings::NativeFormat, QSettings::UserScope,
                                 QStringLiteral("Beatglass"), QStringLiteral("Beatglass")),
      folder_(settings_.value(QStringLiteral("library/folder")).toString()),
      folders_(settings_.value(QStringLiteral("library/folders")).toStringList()) {
  if (folders_.isEmpty() && !folder_.isEmpty()) folders_.push_back(folder_);
  if (!folders_.isEmpty() && folder_.isEmpty()) folder_ = folders_.constFirst();
}

void LibraryPreferences::setFolder(const QString& folder) {
  const auto normalized = folder.trimmed();
  if (folder_ == normalized) return;
  folder_ = normalized;
  settings_.setValue(QStringLiteral("library/folder"), folder_);
  settings_.sync();
  emit folderChanged();
}

void LibraryPreferences::clearFolder() {
  if (folders_.isEmpty()) { setFolder(QString{}); return; }
  folders_.clear();
  setFolder(QString{});
  settings_.setValue(QStringLiteral("library/folders"), folders_);
  settings_.sync();
  emit foldersChanged();
}

void LibraryPreferences::addFolder(const QString& folder) {
  const auto normalized = folder.trimmed();
  if (normalized.isEmpty() || folders_.contains(normalized)) return;
  folders_.push_back(normalized);
  settings_.setValue(QStringLiteral("library/folders"), folders_);
  settings_.sync();
  if (folder_.isEmpty()) setFolder(normalized);
  emit foldersChanged();
}

void LibraryPreferences::removeFolder(int index) {
  if (index < 0 || index >= folders_.size()) return;
  folders_.removeAt(index);
  settings_.setValue(QStringLiteral("library/folders"), folders_);
  settings_.sync();
  setFolder(folders_.isEmpty() ? QString{} : folders_.constFirst());
  emit foldersChanged();
}
