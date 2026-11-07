#pragma once
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/library/detail/tempo_analysis_store.hpp"
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
  /** Owned completion payload. Error text covers initialization, validation and query failures; lookup and analysis carry typed outcomes. */
  struct Result {
    std::variant<std::monostate, Page, Detail, Analysis, Lookup> value; ///< Owned operation payload, if available.
    std::string error; ///< Bridge, initialization or query error.
    bool cancelled{}; ///< Cancellation before a queued request started, or query cancellation.
    std::string association_error; ///< Index update failed after analysis; the analysis outcome remains authoritative.
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
  ~LibraryBridge();
  LibraryBridge(const LibraryBridge&) = delete;
  LibraryBridge& operator=(const LibraryBridge&) = delete;
  /** Replace pending browse work with a bounded collection query. */
  std::shared_ptr<Ticket> entries(beatglass::library_detail::CollectionQuery query, Completion completion);
  /** Replace pending browse work with exact-path detail lookup. */
  std::shared_ptr<Ticket> detail(std::string path, Completion completion);
  /** Replace pending analysis work with snapshot analysis. Epoch must be a nonzero coordinator-issued session epoch. */
  std::shared_ptr<Ticket> analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Look up exactly the supplied content/engine/configuration/mode against a fresh source snapshot, without starting analysis. Epoch is nonzero and session-issued; cached positions are rebound. No result is returned on source mismatch or failed verification. */
  std::shared_ptr<Ticket> lookup(std::filesystem::path path, beatglass::AnalysisIdentity identity, std::uint64_t epoch, Completion completion);
  /** Analyze an indexed path and associate its validated content hash after a hit or commit. Index update is separate from publication; failure is reported in association_error. The normalized key must name the supplied path. */
  std::shared_ptr<Ticket> analyze_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Look up an indexed path's recorded content under the requested tempo engine/configuration; snapshots and revalidates before returning an active value. Missing row/hash is a miss. */
  std::shared_ptr<Ticket> lookup_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Cancel and suppress only browsing and lookup requests. */
  void cancel_browse();
  /** Cancel and suppress only analysis requests; a commit already begun may succeed. */
  void cancel_analysis();
  /** Cancel both lanes. Pending tickets retain cancellation; active tickets retain the actual eventual outcome, including a commit. */
  void cancel();
private:
  std::shared_ptr<State> state_;
};
