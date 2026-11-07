#pragma once
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/library/detail/tempo_analysis_store.hpp"
#include "beatglass/library/detail/offline_archive.hpp"
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
#include "beatglass/library/detail/composite_snapshot_job.hpp"
#endif
#include <functional>
#include <memory>
#include <mutex>
#include <variant>

/** Experimental application-private worker bridge. Construct, request, cancel and destroy on the GUI thread. The supplied dispatcher must enqueue its callable on that same thread, never invoke it inline; its captured objects must outlive the bridge. If dispatch throws, that completion is dropped. One worker runs requests from independent browse and analysis lanes; each lane retains at most one newest pending request and replaces only its own work. A suppressed completion may still have committed before cancellation. Destruction cancels and joins the worker, and suppresses queued deliveries. Blocking filesystem I/O can delay destruction. Not connected to product UI: snapshot identity guarantees remain provisional. */
class LibraryBridge {
  struct State;
public:
  /** Owned metadata row. */
  using Entry = beatglass::library_detail::CollectionEntry;
  /** Bounded owned collection page. */
  using Page = std::vector<Entry>;
  /** Owned exact-path result or absence. */
  using Detail = std::optional<Entry>;
  /** Owned tempo-only terminal result. */
  using Analysis = beatglass::library_detail::StoredSnapshotJobResult;
  /** Identity-bound active-result lookup outcomes. */
  enum class LookupStatus {
    hit, ///< Matching verified cached value.
    miss, ///< No value under this verified identity.
    changed, ///< Source differs or cannot be verified.
    cancelled, ///< Cancellation observed before lookup finished.
    error ///< Storage, input or validation error.
  };
  /** Owned lookup result. A hit is evidence from snapshot revalidation, subject to documented platform races. */
  struct Lookup {
    LookupStatus status{LookupStatus::error}; ///< Terminal outcome.
    std::optional<beatglass::CachedAnalysis> value; ///< Present only for hit.
    std::string diagnostic; ///< Optional failure explanation.
  };
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  /** Worker-confined real composite job outcome; identity is not verified and cannot be cached. */
  using Composite = beatglass::library_detail::CompositeJobResult;
#endif
  /** Owned completion payload. Error text covers initialization, validation and query failures; lookup and analysis carry typed outcomes. */
  struct Result {
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
    std::variant<std::monostate, Page, Detail, Analysis, Lookup, beatglass::library_detail::OfflineReadResult, Composite> value;
#else
    std::variant<std::monostate, Page, Detail, Analysis, Lookup, beatglass::library_detail::OfflineReadResult> value;
#endif ///< Owned operation payload, if available.
    std::string error; ///< Bridge, initialization or query error.
    bool cancelled{}; ///< Cancellation before a queued request started, or query cancellation.
    std::string association_error; ///< Reserved for compatibility; indexed publication now reports failure in its typed outcome.
  };
  /** Thread-safe retained terminal result; remains usable after bridge destruction. Empty means work has not reached a terminal outcome. */
  class Ticket {
  public:
    /** Copy the terminal result if available. */
    std::optional<Result> result() const;
  private:
    friend class LibraryBridge;
    friend struct State;
    void finish(Result result);
    mutable std::mutex mutex_;
    std::optional<Result> result_;
  };
  /** Enqueue a callable to the GUI thread. */
  using Dispatch = std::function<void(std::function<void()>)>;
  /** GUI-thread receiver for one owned result. */
  using Completion = std::function<void(Result)>;
  /** Start the worker; it opens and closes the database on that worker. */
  LibraryBridge(std::string database, Dispatch dispatch);
  /** Inject the same process-owned allocator used by live sources; it must outlive all work. The two-argument constructor is a legacy experimental path. */
  LibraryBridge(std::string database, Dispatch dispatch, std::shared_ptr<beatglass::SourceEpochAllocator> epochs);
  /** Borrow the application-owned allocator. The application must retain it until this bridge is destroyed and its worker is joined. */
  LibraryBridge(std::string database, Dispatch dispatch, beatglass::SourceEpochAllocator& epochs);
  ~LibraryBridge();
  LibraryBridge(const LibraryBridge&) = delete;
  LibraryBridge& operator=(const LibraryBridge&) = delete;
  /** Issue a process-session epoch off the audio callback, or return empty if no allocator was injected or it is exhausted. Use the returned value for run-scoped page lookups. */
  std::optional<std::uint64_t> new_session_epoch();
  /** Allocate a fresh epoch from the injected allocator and start a tempo job. Exhaustion is reported as an error without opening the source. */
  std::shared_ptr<Ticket> start_analysis(std::filesystem::path path, std::string engine, std::string configuration, Completion completion);
  /** Allocate a fresh epoch and start indexed analysis. */
  std::shared_ptr<Ticket> start_indexed_analysis(std::string key, std::filesystem::path path, std::string engine, std::string configuration, Completion completion);
  /** Replace pending browse work with a bounded collection query. */
  std::shared_ptr<Ticket> entries(beatglass::library_detail::CollectionQuery query, Completion completion);
  /** Replace pending browse work with exact-path detail lookup. */
  std::shared_ptr<Ticket> detail(std::string path, Completion completion);
  /** Replace pending analysis work with snapshot analysis. Epoch must be a nonzero coordinator-issued session epoch. */
  std::shared_ptr<Ticket> analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Look up exactly the supplied content/engine/configuration/mode against a fresh source snapshot, without starting analysis. Epoch is nonzero and session-issued; cached positions are rebound. No result is returned on source mismatch or failed verification. */
  std::shared_ptr<Ticket> lookup(std::filesystem::path path, beatglass::AnalysisIdentity identity, std::uint64_t epoch, Completion completion);
  /** Analyze an indexed path and associate its validated content hash after a hit or commit. The active result and index update commit atomically; failure rolls back both. The normalized key must name the supplied path. */
  std::shared_ptr<Ticket> analyze_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Look up an indexed path's recorded content under the requested tempo engine/configuration; snapshots and revalidates before returning an active value. Missing row/hash is a miss. */
  std::shared_ptr<Ticket> lookup_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Fetch an owned, bounded offline page from an indexed path after snapshot verification. Empty run selects the active run; subsequent pages should use the returned run ID. */
  std::shared_ptr<Ticket> offline_page(std::string key, std::filesystem::path path, beatglass::library_detail::OfflineKey identity, std::string run, std::size_t beat_offset, std::size_t beat_count, std::size_t window_offset, std::size_t window_count, std::uint64_t epoch, Completion completion);
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  /** Allocate a fresh epoch and start the non-cacheable composite job. */
  std::shared_ptr<Ticket> start_composite(std::filesystem::path path, Completion completion);
  /** Run real snapshot-backed composite analysis on the analysis lane. Results are owned and explicitly non-cacheable; this never calls the archive. The caller must serialize Essentia construction process-wide and supply a nonzero coordinator epoch. */
  std::shared_ptr<Ticket> analyze_composite(std::filesystem::path path, std::uint64_t epoch, Completion completion);
#endif
  /** Cancel and suppress only browsing and lookup requests. */
  void cancel_browse();
  /** Cancel and suppress only analysis requests; a commit already begun may succeed. */
  void cancel_analysis();
  /** Cancel both lanes. Pending tickets retain cancellation; active tickets retain the actual eventual outcome, including a commit. */
  void cancel();
private:
  std::shared_ptr<State> state_;
};
