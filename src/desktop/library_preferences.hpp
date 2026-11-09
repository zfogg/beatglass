#pragma once
#include <QObject>
#include <QString>
#include <QSettings>
/** GUI-thread Library folder preference persisted in the user's settings. */
class LibraryPreferences final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
public:
  explicit LibraryPreferences(QObject* parent = nullptr);
  QString folder() const { return folder_; }
public slots: void setFolder(const QString& folder);
  void clearFolder();
signals: void folderChanged();
private: QSettings settings_; QString folder_;
};
