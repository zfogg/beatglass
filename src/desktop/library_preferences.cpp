#include "library_preferences.hpp"
LibraryPreferences::LibraryPreferences(QObject* parent) : QObject(parent), settings_(QSettings::NativeFormat, QSettings::UserScope, QStringLiteral("Beatglass"), QStringLiteral("Beatglass")), folder_(settings_.value(QStringLiteral("library/folder")).toString()) {}
void LibraryPreferences::setFolder(const QString& folder) { const auto normalized = folder.trimmed(); if (folder_ == normalized) return; folder_ = normalized; settings_.setValue(QStringLiteral("library/folder"), folder_); settings_.sync(); emit folderChanged(); }
void LibraryPreferences::clearFolder() { setFolder(QString{}); }
