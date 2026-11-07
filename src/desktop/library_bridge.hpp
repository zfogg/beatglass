#pragma once
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/library/detail/tempo_analysis_store.hpp"
#include <functional>
#include <memory>
#include <variant>

/** Experimental application-private worker bridge. Construct, request, cancel and destroy on the GUI thread. The supplied dispatcher must enqueue its callable on that same thread, never invoke it inline; its captured objects must outlive the bridge. If dispatch throws, that completion is dropped. One request runs and one newest request waits; a newer request cancels and suppresses older completions. A suppressed completion may still have committed before cancellation. Destruction cancels and joins the worker, and suppresses queued deliveries. Blocking filesystem I/O can delay destruction. Not connected to product UI: snapshot identity guarantees remain provisional. */
class LibraryBridge {
public:
  using Entry = beatglass::library_detail::CollectionEntry;
  using Page = std::vector<Entry>;
  using Detail = std::optional<Entry>;
  using Analysis = beatglass::library_detail::StoredSnapshotJobResult;
  /** Owned completion payload. Error text covers initialization, validation and query failures; analysis carries its own terminal status. */
  struct Result { std::variant<std::monostate, Page, Detail, Analysis> value; std::string error; bool cancelled{}; };
  using Dispatch = std::function<void(std::function<void()>)>;
  using Completion = std::function<void(Result)>;
  /** Start the worker; it opens and closes the database on that worker. */
  LibraryBridge(std::string database, Dispatch dispatch);
  ~LibraryBridge();
  LibraryBridge(const LibraryBridge&) = delete;
  LibraryBridge& operator=(const LibraryBridge&) = delete;
  /** Replace any pending operation with a bounded collection query. */
  void entries(beatglass::library_detail::CollectionQuery query, Completion completion);
  /** Replace any pending operation with exact-path detail lookup. */
  void detail(std::string path, Completion completion);
  /** Replace any pending operation with snapshot analysis. Epoch must be a nonzero coordinator-issued session epoch. */
  void analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion);
  /** Cancel and suppress the current and pending requests. */
  void cancel();
private:
  struct State;
  std::shared_ptr<State> state_;
};
