#pragma once
#include <memory>
#include <optional>
#include <utility>

/** Provisional GUI-thread-only request state; no worker or source verification is implied.
 * Tokens identify one request by ownership rather than a wrapping counter. Callers must
 * marshal owned results to the GUI thread and suppress callbacks after this object dies.
 * Browse replacement never invalidates an analysis; cancellation must be requested from
 * the corresponding backend separately. At most one analysis terminal is retained until
 * acknowledged. This is not connected to product UI while the backend remains gated.
 */
template <class Page, class Terminal> class CollectionRequests {
public:
  /** Opaque request identity; copies may cross threads without accessing the state. */
  using Token = std::shared_ptr<const char>;
  /** Replace the current browse request and clear its displayed page. */
  Token beginBrowse() { page_.reset(); return browse_ = std::make_shared<const char>(); }
  /** Request backend cancellation before replacing browse state, including when the caller will reject the new request. The callback runs synchronously on the GUI thread. */
  template <class Cancel> Token beginBrowse(Cancel&& cancel) {
    std::forward<Cancel>(cancel)(); return beginBrowse();
  }
  /** Invalidate browse delivery and clear its page; does not cancel backend work. */
  void clearBrowse() { browse_.reset(); page_.reset(); }
  /** Accept an owned page only for the current request. */
  bool finishBrowse(const Token& token, Page page) {
    if (!token || token != browse_) return false;
    browse_.reset(); page_ = std::move(page); return true;
  }
  /** Read the last accepted owned page, or no value after a new request or clear. */
  const std::optional<Page>& page() const { return page_; }
  /** Start one analysis if no analysis or unacknowledged terminal is retained. */
  Token beginAnalysis() {
    if (analysis_ || terminal_) return {};
    return analysis_ = std::make_shared<const char>();
  }
  /** Retain the matching actual terminal outcome once; cancellation is not assumed. */
  bool finishAnalysis(const Token& token, Terminal result) {
    if (!token || token != analysis_) return false;
    analysis_.reset(); terminal_ = std::move(result); return true;
  }
  /** Read the retained owned terminal until acknowledged. */
  const std::optional<Terminal>& terminal() const { return terminal_; }
  /** Discard a consumed terminal, allowing the next analysis. */
  void acknowledgeAnalysis() { terminal_.reset(); }
  /** Move the retained terminal to its GUI owner and allow the next analysis. */
  std::optional<Terminal> takeAnalysis() { auto result = std::move(terminal_); terminal_.reset(); return result; }
private:
  Token browse_, analysis_;
  std::optional<Page> page_;
  std::optional<Terminal> terminal_;
};
