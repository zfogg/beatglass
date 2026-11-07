#pragma once
#include "library_bridge.hpp"
#include "collection_requests.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <QUrl>
#include <memory>

/** Experimental GUI-thread adapter for the worker-owned library. Construct and destroy on the GUI thread. Results are queued to this QObject; destruction joins the worker and Qt discards queued callbacks. Page data and provisional display rows are owned and bounded. Cached analysis remains hidden while verification races remain. */
class CollectionView : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(QString analysisStatus READ analysisStatus NOTIFY changed)
  Q_PROPERTY(bool compositeAvailable READ compositeAvailable CONSTANT)
  Q_PROPERTY(bool analysisActive READ analysisActive NOTIFY changed)
  Q_PROPERTY(QString provisionalSummary READ provisionalSummary NOTIFY changed)
  Q_PROPERTY(QVariantList provisionalWindows READ provisionalWindows NOTIFY changed)
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
  /** Whether this build includes the private uncached composite worker. */
  bool compositeAvailable() const;
  /** Whether a worker terminal is still pending. */
  bool analysisActive() const { return bool(analysisTicket_); }
  /** A provisional explanation, including explicitly unknown track key. */
  QString provisionalSummary() const;
  /** Up to 64 owned display rows for overlapping local evidence, with unknown strengths kept explicit. */
  QVariantList provisionalWindows() const;
  /** Select a local source, invalidate old evidence and request cancellation of any active job. */
  Q_INVOKABLE void selectSource(const QUrl& url);
  /** Run uncached provisional composite analysis for the current selection, if supported. */
  Q_INVOKABLE bool analyzeSelected();
  /** Cancel active analysis and immediately hide its evidence. */
  Q_INVOKABLE void cancelSelected();
  /** Replace the current browse query; older pages are never presented. */
  Q_INVOKABLE void refresh(const QString& search);
  /** Submit a real indexed tempo job with the exact runtime identity and a fresh process epoch. Unknown identity or exhausted epochs reject the request. The source is not modified. */
  bool analyzeIndexed(const QString& path, const QString& engine, const QString& configuration);
  /** Request cancellation; a commit already started may still succeed and its retained ticket remains authoritative. */
  void cancelAnalysis();
  /** Look up an indexed file under the exact runtime identity and a fresh process epoch. A hit remains provisional and its value is not exposed. This replaces pending browse work. */
  void lookupIndexed(const QString& path, const QString& engine, const QString& configuration);
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  /** Start uncached real composite analysis with a fresh process epoch. The caller must ensure process-wide serialized Essentia construction. The raw owned result is provisional and is not exposed directly to QML. */
  bool analyzeComposite(const QString& path);
  /** Read the last owned, explicitly non-cacheable composite evidence. A new analysis or destruction invalidates the reference. */
  const std::optional<beatglass::UnverifiedMusicAnalysis>& compositeEvidence() const { return compositeEvidence_; }
#endif
  /** Request a bounded page for a verified identity. An empty run starts a new process epoch; continuation requires the previously returned run and keeps its epoch. Results remain provisional and hidden from product UI. */
  void requestOfflinePage(const QString& path, beatglass::library_detail::OfflineKey identity, const std::string& run, std::size_t beatOffset, std::size_t beatCount, std::size_t windowOffset, std::size_t windowCount);
  /** Read the last owned provisional page; invalidated immediately by another browse, lookup, or page request. */
  const std::optional<beatglass::library_detail::OfflinePage>& offlinePage() const { return offlinePage_; }
signals:
  /** Page or query status changed on the GUI thread. */
  void changed();
private:
  friend class CollectionViewTest;
  using Requests = CollectionRequests<LibraryBridge::Page, LibraryBridge::Result>;
  Requests requests_;
  beatglass::SourceEpochAllocator& epochs_;
  std::optional<std::uint64_t> pageEpoch_;
  QString pagePath_;
  QVariantList rows_;
  QString status_;
  QString analysisStatus_;
  QString selectedPath_;
  QString submittedPath_;
  std::shared_ptr<const char> selectionToken_;
  std::shared_ptr<const char> submittedSelection_;
  std::shared_ptr<const char> evidenceSelection_;
  QTimer poll_;
  Requests::Token analysisToken_;
  std::shared_ptr<LibraryBridge::Ticket> analysisTicket_;
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  std::optional<beatglass::UnverifiedMusicAnalysis> compositeEvidence_;
#endif
  std::optional<beatglass::library_detail::OfflinePage> offlinePage_;
  void pollAnalysis();
  std::unique_ptr<LibraryBridge> worker_;
};
