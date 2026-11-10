#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>
/** GUI-thread Library directory list persisted in the user's settings. */
class LibraryPreferences final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
  Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)
public:
  explicit LibraryPreferences(QObject* parent = nullptr);
  QString folder() const { return folder_; }
  QStringList folders() const { return folders_; }
public slots: void setFolder(const QString& folder);
  void clearFolder();
  Q_INVOKABLE void addFolder(const QString& folder);
  Q_INVOKABLE void removeFolder(int index);
signals: void folderChanged();
  void foldersChanged();
private: QSettings settings_; QString folder_; QStringList folders_;
};
