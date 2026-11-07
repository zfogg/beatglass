#pragma once
#include "library_bridge.hpp"
#include "collection_requests.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
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
  /** Open the provisional database on the worker. The shared process epoch allocator must outlive this object and its joined worker. */
  explicit CollectionView(const QString& database, beatglass::SourceEpochAllocator& epochs, QObject* parent = nullptr);
  ~CollectionView() override;
  /** Return a copy of the current bounded page. */
  QVariantList rows() const { return rows_; }
  /** Return the current query status. */
  QString status() const { return status_; }
  /** Return the analysis terminal or in-progress status; it never contains an analysis value. */
  QString analysisStatus() const { return analysisStatus_; }
  /** Replace the current browse query; older pages are never presented. */
  Q_INVOKABLE void refresh(const QString& search);
  /** Submit a real indexed tempo job with the exact runtime identity and a fresh process epoch. Unknown identity or exhausted epochs reject the request. The source is not modified. */
  bool analyzeIndexed(const QString& path, const QString& engine, const QString& configuration);
  /** Request cancellation; a commit already started may still succeed and its retained ticket remains authoritative. */
  void cancelAnalysis();
  /** Look up an indexed file under the exact runtime identity and a fresh process epoch. A hit remains provisional and its value is not exposed. This replaces pending browse work. */
  void lookupIndexed(const QString& path, const QString& engine, const QString& configuration);
  /** Request a bounded page for a verified identity. An empty run starts a new process epoch; continuation requires the previously returned run and keeps its epoch. Results remain provisional and hidden from product UI. */
  void requestOfflinePage(const QString& path, beatglass::library_detail::OfflineKey identity, const std::string& run, std::size_t beatOffset, std::size_t beatCount, std::size_t windowOffset, std::size_t windowCount);
  /** Read the last owned provisional page; invalidated immediately by another browse, lookup, or page request. */
  const std::optional<beatglass::library_detail::OfflinePage>& offlinePage() const { return offlinePage_; }
signals:
  /** Page or query status changed on the GUI thread. */
  void changed();
private:
  using Requests = CollectionRequests<LibraryBridge::Page, LibraryBridge::Result>;
  Requests requests_;
  beatglass::SourceEpochAllocator& epochs_;
  std::optional<std::uint64_t> pageEpoch_;
  QString pagePath_;
  QVariantList rows_;
  QString status_;
  QString analysisStatus_;
  QTimer poll_;
  Requests::Token analysisToken_;
  std::shared_ptr<LibraryBridge::Ticket> analysisTicket_;
  std::optional<beatglass::library_detail::OfflinePage> offlinePage_;
  void pollAnalysis();
  std::unique_ptr<LibraryBridge> worker_;
};
