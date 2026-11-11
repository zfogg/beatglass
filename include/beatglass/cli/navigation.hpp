#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace beatglass::cli {

/** Terminal workspace pane selected by the v2 navigation model. */
enum class Pane { library, analysis, now_playing };

/** Layout policy derived from the terminal width. */
enum class Layout { compact, two_column };

/** Modal interaction currently owning keyboard focus. */
enum class Modal { none, command_palette, help };

/** A stable command exposed by the command palette and keyboard shortcuts. */
struct Command {
  std::string id;
  std::string label;
  std::string shortcut;
};

/** Pure navigation state; UI code owns rendering and audio work separately. */
class NavigationModel {
 public:
  NavigationModel();

  /** Update pane selection while preserving modal focus. */
  void select(Pane pane) noexcept;
  /** Open or close the command palette. */
  void toggle_palette() noexcept;
  /** Close any modal overlay. */
  void dismiss_modal() noexcept;
  /** Update terminal layout using a conservative two-column breakpoint. */
  void resize(int width) noexcept;
  /** Move the palette cursor by a bounded delta. */
  void move_command(int delta) noexcept;
  /** Filter palette commands by case-insensitive label or id. */
  void set_query(std::string query);

  [[nodiscard]] Pane pane() const noexcept { return pane_; }
  [[nodiscard]] Layout layout() const noexcept { return layout_; }
  [[nodiscard]] Modal modal() const noexcept { return modal_; }
  [[nodiscard]] int command_index() const noexcept { return command_index_; }
  [[nodiscard]] const std::string& query() const noexcept { return query_; }
  [[nodiscard]] std::vector<Command> visible_commands() const;

 private:
  Pane pane_{Pane::library};
  Layout layout_{Layout::two_column};
  Modal modal_{Modal::none};
  int command_index_{0};
  std::string query_;
  std::vector<Command> commands_;
};

}  // namespace beatglass::cli
