#pragma once
#include "library_bridge.hpp"
#include "collection_requests.hpp"
#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <memory>

/** Experimental GUI-thread adapter for the worker-owned library. Construct and destroy on the GUI thread. Results are queued to this QObject; destruction joins the worker and Qt discards queued callbacks. Page data is owned and bounded. No cached analysis value is exposed while verification races remain. */
class CollectionView : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(QString analysisStatus READ analysisStatus NOTIFY changed)
public:
  /** Open the provisional database on the worker, not on the GUI thread. */
  explicit CollectionView(const QString& database, QObject* parent = nullptr);
  ~CollectionView() override;
  /** Return a copy of the current bounded page. */
  QVariantList rows() const { return rows_; }
  /** Return the current query status. */
  QString status() const { return status_; }
  /** Return the analysis terminal or in-progress status; it never contains an analysis value. */
  QString analysisStatus() const { return analysisStatus_; }
  /** Replace the current browse query; older pages are never presented. */
  Q_INVOKABLE void refresh(const QString& search);
  /** Submit a real indexed tempo job with the exact runtime identity and a nonzero epoch from the process-owned coordinator. The caller must not invent identities or reuse an epoch. Reject a second job until the first terminal has been observed. The source is not modified. */
  bool analyzeIndexed(const QString& path, const QString& engine, const QString& configuration, quint64 epoch);
  /** Request cancellation; a commit already started may still succeed and its retained ticket remains authoritative. */
  void cancelAnalysis();
  /** Look up an indexed file under the exact runtime identity and a nonzero process-coordinator-issued epoch. A hit is reported as provisional and its value is not exposed. This replaces pending browse work. */
  void lookupIndexed(const QString& path, const QString& engine, const QString& configuration, quint64 epoch);
signals:
  /** Page or query status changed on the GUI thread. */
  void changed();
private:
  using Requests = CollectionRequests<LibraryBridge::Page, LibraryBridge::Result>;
  Requests requests_;
  QVariantList rows_;
  QString status_;
  QString analysisStatus_;
  QTimer poll_;
  Requests::Token analysisToken_;
  std::shared_ptr<LibraryBridge::Ticket> analysisTicket_;
  void pollAnalysis();
  std::unique_ptr<LibraryBridge> worker_;
};
