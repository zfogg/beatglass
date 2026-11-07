#include "collection_view.hpp"
#include <QMetaObject>
#include <stdexcept>
#include <filesystem>

namespace {
std::filesystem::path nativePath(const QString& path) {
#ifdef _WIN32
  return std::filesystem::path(path.toStdWString());
#else
  return std::filesystem::u8path(path.toUtf8().toStdString());
#endif
}
}

CollectionView::CollectionView(const QString& database, beatglass::SourceEpochAllocator& epochs, QObject* parent) : QObject(parent), epochs_(epochs) {
  worker_ = std::make_unique<LibraryBridge>(database.toUtf8().toStdString(), [this](auto task) {
    if (!QMetaObject::invokeMethod(this, std::move(task), Qt::QueuedConnection))
      throw std::runtime_error("GUI dispatch failed");
  });
  poll_.setInterval(50);
  connect(&poll_, &QTimer::timeout, this, &CollectionView::pollAnalysis);
}
CollectionView::~CollectionView() { worker_.reset(); }
bool CollectionView::compositeAvailable() const {
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  return true;
#else
  return false;
#endif
}
QString CollectionView::provisionalSummary() const {
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  if (compositeEvidence_ && evidenceSelection_ && evidenceSelection_ == selectionToken_) {
    const auto& value = compositeEvidence_->value;
    return QStringLiteral("Provisional uncached estimates; source and dependency identity are not fully verified. Track-level key: %1. %2 local windows (showing up to 64).")
      .arg(value.validated_track_key ? QString::fromStdString(value.validated_track_key->label) : QStringLiteral("Unknown"))
      .arg(static_cast<qulonglong>(value.tonal_windows.size()));
  }
#endif
  return QStringLiteral("No provisional analysis for this selection. Track-level key: Unknown.");
}
QVariantList CollectionView::provisionalWindows() const {
  QVariantList rows;
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  if (!compositeEvidence_ || !evidenceSelection_ || evidenceSelection_ != selectionToken_) return rows;
  const auto& windows = compositeEvidence_->value.tonal_windows;
  for (std::size_t i = 0; i < windows.size() && i < 64; ++i) {
    const auto& window = windows[i];
    auto strength = [](const std::optional<beatglass::OfflineEstimate>& estimate) {
      return estimate && estimate->strength ? QString::number(*estimate->strength, 'g', 4) : QStringLiteral("Unknown");
    };
    QVariantMap row;
    row.insert(QStringLiteral("range"), QStringLiteral("Frames [%1, %2)").arg(static_cast<qulonglong>(window.start_frame)).arg(static_cast<qulonglong>(window.end_frame)));
    row.insert(QStringLiteral("key"), window.rolling_key ? QString::fromStdString(window.rolling_key->label) : QStringLiteral("Unknown"));
    row.insert(QStringLiteral("keyFrom"), window.rolling_key_from ? QString::number(static_cast<qulonglong>(*window.rolling_key_from)) : QStringLiteral("Unknown"));
    row.insert(QStringLiteral("keyStrength"), strength(window.rolling_key));
    row.insert(QStringLiteral("chord"), window.local_chord ? QString::fromStdString(window.local_chord->label) : QStringLiteral("Unknown"));
    row.insert(QStringLiteral("chordStrength"), strength(window.local_chord));
    rows.push_back(std::move(row));
  }
#endif
  return rows;
}
void CollectionView::selectSource(const QUrl& url) {
  selectionToken_ = std::make_shared<const char>(); evidenceSelection_.reset();
  selectedPath_ = url.isLocalFile() ? url.toLocalFile() : QString();
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  compositeEvidence_.reset();
#endif
  if (analysisTicket_) worker_->cancel_analysis();
  analysisStatus_ = selectedPath_.isEmpty() ? QStringLiteral("Select a local file.") : QStringLiteral("Ready for provisional analysis.");
  emit changed();
}
bool CollectionView::analyzeSelected() {
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  if (selectedPath_.isEmpty() || !selectionToken_ || analysisTicket_) return false;
  submittedSelection_ = selectionToken_;
  submittedPath_ = selectedPath_;
  if (analyzeComposite(selectedPath_)) return true;
  submittedSelection_.reset(); submittedPath_.clear();
#endif
  return false;
}
void CollectionView::cancelSelected() {
  evidenceSelection_.reset();
  if (submittedSelection_) submittedSelection_ = std::make_shared<const char>();
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  compositeEvidence_.reset();
#endif
  cancelAnalysis();
  analysisStatus_ = analysisTicket_ ? QStringLiteral("Cancellation requested; result hidden.") : QStringLiteral("Result hidden.");
  emit changed();
}
void CollectionView::refresh(const QString& search) {
  auto token = requests_.beginBrowse(); offlinePage_.reset(); pageEpoch_.reset(); pagePath_.clear(); rows_.clear(); status_ = QStringLiteral("Loading indexed files…");
  beatglass::library_detail::CollectionQuery query; query.search = search.toUtf8().toStdString(); query.limit = 100;
  worker_->entries(std::move(query), [this, token](LibraryBridge::Result result) {
    if (!result.error.empty() || result.cancelled) {
      if (!requests_.finishBrowse(token, {})) return;
      status_ = result.cancelled ? QStringLiteral("Query cancelled.") : QStringLiteral("Library error: %1").arg(QString::fromStdString(result.error));
    } else if (auto page = std::get_if<LibraryBridge::Page>(&result.value)) {
      if (!requests_.finishBrowse(token, std::move(*page))) return;
      for (const auto& entry : *requests_.page()) {
        QVariantMap row; row.insert(QStringLiteral("path"), QString::fromStdString(entry.path));
        row.insert(QStringLiteral("title"), QString::fromStdString(entry.title));
        row.insert(QStringLiteral("artist"), QString::fromStdString(entry.artist)); rows_.push_back(row);
      }
      status_ = rows_.isEmpty() ? QStringLiteral("No indexed files.") : QStringLiteral("Showing up to 100 indexed files.");
    } else {
      if (!requests_.finishBrowse(token, {})) return;
      status_ = QStringLiteral("Unexpected library response.");
    }
    emit changed();
  });
  emit changed();
}
bool CollectionView::analyzeIndexed(const QString& path, const QString& engine, const QString& configuration) {
  if (path.isEmpty() || engine.isEmpty() || configuration.isEmpty() || analysisTicket_) return false;
  auto epoch = epochs_.next(); if (!epoch) { analysisStatus_ = QStringLiteral("Analysis unavailable: epoch limit reached."); emit changed(); return false; }
  analysisToken_ = requests_.beginAnalysis();
  if (!analysisToken_) return false;
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  compositeEvidence_.reset();
#endif
  analysisStatus_ = QStringLiteral("Analysis running.");
  analysisTicket_ = worker_->analyze_indexed(path.toUtf8().toStdString(), nativePath(path), engine.toUtf8().toStdString(), configuration.toUtf8().toStdString(), *epoch, {});
  poll_.start(); emit changed(); return true;
}
void CollectionView::cancelAnalysis() {
  if (!analysisTicket_) return;
  worker_->cancel_analysis(); analysisStatus_ = QStringLiteral("Cancellation requested; awaiting terminal outcome."); emit changed();
}
void CollectionView::pollAnalysis() {
  if (!analysisTicket_) return;
  auto result = analysisTicket_->result(); if (!result) return;
  poll_.stop(); analysisTicket_.reset();
  if (!requests_.finishAnalysis(analysisToken_, std::move(*result))) return;
  auto held = requests_.takeAnalysis();
  auto& outcome = *held;
  if (submittedSelection_ && (submittedSelection_ != selectionToken_ || submittedPath_ != selectedPath_)) {
    if (analysisStatus_ == QStringLiteral("Cancellation requested; result hidden.")) analysisStatus_ = QStringLiteral("Analysis finished; result hidden.");
    analysisToken_.reset(); submittedSelection_.reset(); submittedPath_.clear(); emit changed(); return;
  }
  if (!outcome.error.empty()) analysisStatus_ = QStringLiteral("Analysis error: %1").arg(QString::fromStdString(outcome.error));
  else if (outcome.cancelled) analysisStatus_ = QStringLiteral("Analysis cancelled.");
  else if (const auto* analysis = std::get_if<LibraryBridge::Analysis>(&outcome.value)) {
    using S = beatglass::library_detail::FileJobStatus;
    switch (analysis->status) {
      case S::committed: analysisStatus_ = QStringLiteral("Analysis committed; result remains gated."); break;
      case S::cache_hit: analysisStatus_ = QStringLiteral("Existing analysis found; result remains gated."); break;
      case S::cancelled: analysisStatus_ = QStringLiteral("Analysis cancelled."); break;
      case S::stale_input: analysisStatus_ = QStringLiteral("Source changed during analysis."); break;
      case S::error: analysisStatus_ = QStringLiteral("Analysis error: %1; recovery run: %2").arg(QString::fromStdString(analysis->diagnostic), QString::fromStdString(analysis->recovery_run_id)); break;
    }
    if (!outcome.association_error.empty()) analysisStatus_ += QStringLiteral(" Index association failed: %1").arg(QString::fromStdString(outcome.association_error));
  }
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
  else if (auto* composite = std::get_if<LibraryBridge::Composite>(&outcome.value)) {
    using S = beatglass::library_detail::CompositeJobStatus;
    switch (composite->status) {
      case S::complete:
        if (composite->value && !composite->value->cache_reusable()) { compositeEvidence_ = std::move(composite->value); evidenceSelection_ = submittedSelection_; analysisStatus_ = QStringLiteral("Provisional analysis complete; caching is disabled."); }
        else analysisStatus_ = QStringLiteral("Composite error: invalid result.");
        break;
      case S::cancelled: analysisStatus_ = QStringLiteral("Composite analysis cancelled."); break;
      case S::stale_input: analysisStatus_ = QStringLiteral("Source changed during composite analysis."); break;
      case S::error: analysisStatus_ = QStringLiteral("Composite error: %1").arg(QString::fromStdString(composite->diagnostic)); break;
    }
  }
#endif
  else analysisStatus_ = QStringLiteral("Unexpected analysis response.");
  analysisToken_.reset(); submittedSelection_.reset(); submittedPath_.clear(); emit changed();
}
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
bool CollectionView::analyzeComposite(const QString& path) {
  if (path.isEmpty() || analysisTicket_) return false;
  auto epoch = epochs_.next();
  if (!epoch) { analysisStatus_ = QStringLiteral("Analysis unavailable: epoch limit reached."); emit changed(); return false; }
  analysisToken_ = requests_.beginAnalysis(); if (!analysisToken_) return false;
  compositeEvidence_.reset(); evidenceSelection_.reset(); analysisStatus_ = QStringLiteral("Composite analysis running.");
  analysisTicket_ = worker_->analyze_composite(nativePath(path), *epoch, {});
  poll_.start(); emit changed(); return true;
}
#endif
void CollectionView::lookupIndexed(const QString& path, const QString& engine, const QString& configuration) {
  auto token = requests_.beginBrowse(); offlinePage_.reset(); pageEpoch_.reset(); pagePath_.clear(); rows_.clear();
  if (path.isEmpty() || engine.isEmpty() || configuration.isEmpty()) { status_ = QStringLiteral("Lookup unavailable: identity or epoch missing."); emit changed(); return; }
  auto epoch = epochs_.next();
  if (!epoch) { status_ = QStringLiteral("Lookup unavailable: identity or epoch missing."); emit changed(); return; }
  status_ = QStringLiteral("Checking indexed analysis…");
  worker_->lookup_indexed(path.toUtf8().toStdString(), nativePath(path), engine.toUtf8().toStdString(), configuration.toUtf8().toStdString(), *epoch, [this, token](LibraryBridge::Result result) {
    if (!requests_.finishBrowse(token, {})) return;
    if (!result.error.empty()) status_ = QStringLiteral("Lookup error: %1").arg(QString::fromStdString(result.error));
    else if (result.cancelled) status_ = QStringLiteral("Lookup cancelled.");
    else if (const auto* lookup = std::get_if<LibraryBridge::Lookup>(&result.value)) {
      using S = LibraryBridge::LookupStatus;
      switch (lookup->status) {
        case S::hit: status_ = QStringLiteral("Provisional analysis found; result remains gated."); break;
        case S::miss: status_ = QStringLiteral("No indexed analysis."); break;
        case S::changed: status_ = QStringLiteral("Source changed or cannot be verified."); break;
        case S::cancelled: status_ = QStringLiteral("Lookup cancelled."); break;
        case S::error: status_ = QStringLiteral("Lookup error: %1").arg(QString::fromStdString(lookup->diagnostic)); break;
      }
    } else status_ = QStringLiteral("Unexpected lookup response.");
    emit changed();
  });
  emit changed();
}
void CollectionView::requestOfflinePage(const QString& path, beatglass::library_detail::OfflineKey identity, const std::string& run, std::size_t beatOffset, std::size_t beatCount, std::size_t windowOffset, std::size_t windowCount) {
  const bool continuation = !run.empty() && offlinePage_ && offlinePage_->run_id == run && pagePath_ == path && pageEpoch_;
  const bool identityPresent = !path.isEmpty() && !identity.content_hash.empty() && !identity.engine_version.empty() && !identity.configuration_id.empty();
  auto epoch = identityPresent ? (run.empty() ? epochs_.next() : (continuation ? pageEpoch_ : std::nullopt)) : std::nullopt;
  auto token = requests_.beginBrowse(); offlinePage_.reset(); rows_.clear();
  if (!epoch) { pageEpoch_.reset(); pagePath_.clear(); status_ = QStringLiteral("Offline page unavailable: identity, run, or epoch missing."); emit changed(); return; }
  pageEpoch_ = *epoch; pagePath_ = path; status_ = QStringLiteral("Checking offline page…");
  worker_->offline_page(path.toUtf8().toStdString(), nativePath(path), std::move(identity), run, beatOffset, beatCount, windowOffset, windowCount, *epoch, [this, token](LibraryBridge::Result result) {
    if (!requests_.finishBrowse(token, {})) return;
    if (!result.error.empty()) status_ = QStringLiteral("Offline page error: %1").arg(QString::fromStdString(result.error));
    else if (result.cancelled) status_ = QStringLiteral("Offline page cancelled.");
    else if (auto* read = std::get_if<beatglass::library_detail::OfflineReadResult>(&result.value)) {
      using S = beatglass::library_detail::OfflineReadStatus;
      switch (read->status) {
        case S::hit:
          if (read->page) { offlinePage_ = std::move(read->page); status_ = QStringLiteral("Provisional offline page received; result remains gated."); }
          else status_ = QStringLiteral("Offline page error: missing page.");
          break;
        case S::miss: status_ = QStringLiteral("No offline analysis."); break;
        case S::changed: status_ = QStringLiteral("Offline source or active run changed."); break;
        case S::cancelled: status_ = QStringLiteral("Offline page cancelled."); break;
        case S::error: status_ = QStringLiteral("Offline page error: %1").arg(QString::fromStdString(read->diagnostic)); break;
      }
    } else status_ = QStringLiteral("Unexpected offline page response.");
    emit changed();
  });
  emit changed();
}
