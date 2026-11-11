#include "beatglass/cli/navigation.hpp"

#include <algorithm>
#include <cctype>

namespace beatglass::cli {
namespace {
std::string lower(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}

NavigationModel::NavigationModel()
    // The command palette is deliberately data-driven so UI renderers can share it.
    : commands_{{"library", "Go to library", "1"}, {"analysis", "Show analysis", "2"},
                 {"now-playing", "Show now playing", "3"}, {"analyze", "Analyze selected track", "Enter"},
                 {"help", "Show keyboard help", "?"}} {}

void NavigationModel::select(Pane pane) noexcept { pane_ = pane; }
void NavigationModel::toggle_palette() noexcept {
  modal_ = modal_ == Modal::command_palette ? Modal::none : Modal::command_palette;
  command_index_ = 0;
}
void NavigationModel::dismiss_modal() noexcept { modal_ = Modal::none; }
void NavigationModel::resize(int width) noexcept { layout_ = width >= 96 ? Layout::two_column : Layout::compact; }
void NavigationModel::move_command(int delta) noexcept {
  const auto count = static_cast<int>(visible_commands().size());
  if (count == 0) { command_index_ = 0; return; }
  command_index_ = std::clamp(command_index_ + delta, 0, count - 1);
}
void NavigationModel::set_query(std::string query) {
  query_ = std::move(query);
  command_index_ = 0;
}
std::vector<Command> NavigationModel::visible_commands() const {
  const auto needle = lower(query_);
  std::vector<Command> result;
  for (const auto& command : commands_) {
    if (needle.empty() || lower(command.label).find(needle) != std::string::npos || lower(command.id).find(needle) != std::string::npos)
      result.push_back(command);
  }
  return result;
}
}  // namespace beatglass::cli
