#pragma once
#include "library_bridge.hpp"
#include "collection_requests.hpp"
#include <QObject>
#include <QVariantList>
#include <memory>

/** Experimental GUI-thread adapter for the worker-owned library. Construct and destroy on the GUI thread. Results are queued to this QObject; destruction joins the worker and Qt discards queued callbacks. Page data is owned and bounded. No cached analysis value is exposed while verification races remain. */
class CollectionView : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
public:
  /** Open the provisional database on the worker, not on the GUI thread. */
  explicit CollectionView(const QString& database, QObject* parent = nullptr);
  ~CollectionView() override;
  /** Return a copy of the current bounded page. */
  QVariantList rows() const { return rows_; }
  /** Return the current query status. */
  QString status() const { return status_; }
  /** Replace the current browse query; older pages are never presented. */
  Q_INVOKABLE void refresh(const QString& search);
signals:
  /** Page or query status changed on the GUI thread. */
  void changed();
private:
  using Requests = CollectionRequests<LibraryBridge::Page, LibraryBridge::Result>;
  Requests requests_;
  QVariantList rows_;
  QString status_;
  std::unique_ptr<LibraryBridge> worker_;
};
