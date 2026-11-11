#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QVariantList>
/** GUI-thread-only versioned Library root preference model. */
class LibraryPreferences final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
  Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)
  Q_PROPERTY(QVariantList roots READ roots NOTIFY rootsChanged)
  Q_PROPERTY(int modelVersion READ modelVersion CONSTANT)
public:
  explicit LibraryPreferences(QObject* parent = nullptr);
  QString folder() const { return folder_; }
  QStringList folders() const;
  QVariantList roots() const { return roots_; }
  int modelVersion() const { return 1; }
public slots: void setFolder(const QString& folder);
  void clearFolder();
  Q_INVOKABLE QString addFolder(const QString& path, const QString& label = {});
  Q_INVOKABLE bool removeFolder(const QString& sourceId);
  Q_INVOKABLE bool setFolderLabel(const QString& sourceId, const QString& label);
  Q_INVOKABLE bool setFolderScanState(const QString& sourceId, const QString& state, const QString& error = {});
signals: void folderChanged();
  void foldersChanged();
  void rootsChanged();
private:
  void load(); void persist();
  static QString sourceIdForPath(const QString& path);
  QSettings settings_; QString folder_; QVariantList roots_;
};
