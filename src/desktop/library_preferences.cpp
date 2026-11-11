#include "library_preferences.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

LibraryPreferences::LibraryPreferences(QObject* parent)
    : QObject(parent), settings_(QSettings::NativeFormat, QSettings::UserScope,
                                 QStringLiteral("Beatglass"), QStringLiteral("Beatglass")) { load(); }

QStringList LibraryPreferences::folders() const {
  QStringList result;
  for (const auto& root : roots_) result.append(root.toMap().value(QStringLiteral("path")).toString());
  return result;
}

void LibraryPreferences::load() {
  const auto doc = QJsonDocument::fromJson(settings_.value(QStringLiteral("library/model")).toByteArray());
  if (doc.isObject() && doc.object().value(QStringLiteral("version")).toInt() == modelVersion())
    for (const auto& value : doc.object().value(QStringLiteral("roots")).toArray())
      if (value.isObject()) roots_.append(value.toObject().toVariantMap());
  if (roots_.isEmpty()) {
    for (const auto& path : settings_.value(QStringLiteral("library/folders")).toStringList()) addFolder(path);
    if (roots_.isEmpty()) addFolder(settings_.value(QStringLiteral("library/folder")).toString());
  }
  if (!roots_.isEmpty()) folder_ = roots_.first().toMap().value(QStringLiteral("path")).toString();
}

void LibraryPreferences::persist() {
  QJsonArray roots;
  for (const auto& root : roots_) roots.append(QJsonObject::fromVariantMap(root.toMap()));
  settings_.setValue(QStringLiteral("library/model"), QJsonDocument(QJsonObject{{QStringLiteral("version"), modelVersion()}, {QStringLiteral("roots"), roots}}).toJson(QJsonDocument::Compact));
  settings_.setValue(QStringLiteral("library/folders"), folders());
  settings_.sync();
}

QString LibraryPreferences::sourceIdForPath(const QString& path) {
  const auto info = QFileInfo(path);
  const auto stable = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
  return QString::fromLatin1(QCryptographicHash::hash(stable.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString LibraryPreferences::addFolder(const QString& path, const QString& label) {
  const auto trimmed = path.trimmed();
  if (trimmed.isEmpty()) return {};
  const auto normalized = QDir::cleanPath(trimmed);
  const auto id = sourceIdForPath(normalized);
  for (const auto& root : roots_)
    if (root.toMap().value(QStringLiteral("sourceId")).toString() == id) return id;
  roots_.append(QVariantMap{{QStringLiteral("sourceId"), id}, {QStringLiteral("path"), normalized},
                            {QStringLiteral("label"), label.trimmed().isEmpty() ? QFileInfo(normalized).fileName() : label.trimmed()},
                            {QStringLiteral("scanState"), QStringLiteral("never")}, {QStringLiteral("lastError"), QString()}});
  if (folder_.isEmpty()) { folder_ = normalized; emit folderChanged(); }
  persist(); emit rootsChanged(); emit foldersChanged(); return id;
}

bool LibraryPreferences::removeFolder(const QString& id) {
  for (int i = 0; i < roots_.size(); ++i) if (roots_[i].toMap().value(QStringLiteral("sourceId")).toString() == id) {
    const auto removed = roots_[i].toMap().value(QStringLiteral("path")).toString();
    roots_.removeAt(i);
    if (folder_ == removed) folder_ = roots_.isEmpty() ? QString{} : roots_.first().toMap().value(QStringLiteral("path")).toString();
    persist(); emit rootsChanged(); emit foldersChanged(); emit folderChanged(); return true;
  }
  return false;
}
bool LibraryPreferences::moveFolder(const QString& id, int delta) {
  const int from = [&] { for (int i = 0; i < roots_.size(); ++i) if (roots_[i].toMap().value(QStringLiteral("sourceId")).toString() == id) return i; return -1; }();
  const int to = from + delta;
  if (from < 0 || delta == 0 || to < 0 || to >= roots_.size()) return false;
  roots_.move(from, to); persist(); emit rootsChanged(); emit foldersChanged(); return true;
}

bool LibraryPreferences::setFolderLabel(const QString& id, const QString& label) { for (auto& value : roots_) if (value.toMap().value(QStringLiteral("sourceId")).toString() == id) { auto map = value.toMap(); map[QStringLiteral("label")] = label.trimmed(); value = map; persist(); emit rootsChanged(); return true; } return false; }
bool LibraryPreferences::setFolderScanState(const QString& id, const QString& state, const QString& error) { for (auto& value : roots_) if (value.toMap().value(QStringLiteral("sourceId")).toString() == id) { auto map = value.toMap(); map[QStringLiteral("scanState")] = state; map[QStringLiteral("lastError")] = error; value = map; persist(); emit rootsChanged(); return true; } return false; }
void LibraryPreferences::setFolder(const QString& folder) { const auto normalized = folder.trimmed(); if (folder_ == normalized) return; folder_ = normalized; settings_.setValue(QStringLiteral("library/folder"), normalized); if (!normalized.isEmpty()) addFolder(normalized); else { persist(); emit rootsChanged(); } emit folderChanged(); }
void LibraryPreferences::clearFolder() { if (!folder_.isEmpty()) removeFolder(sourceIdForPath(folder_)); }
