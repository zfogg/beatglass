#include "beatglass/core/file_analysis.hpp"
#include "beatglass/cli/navigation.hpp"
#include "state_label.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace ftxui;
struct Track { std::filesystem::path path; std::string title; };
enum class Popup { none, folder, output, colors, settings, permissions, recovery, confirm_remove };
std::string format_bpm(const std::optional<float>& bpm) {
  if (!bpm) return "—";
  std::ostringstream out; out << std::fixed << std::setprecision(1) << *bpm; return out.str();
}
Element metric(std::string label, std::string value, Color color) {
  return vbox({text(std::move(label)) | color(Color::GrayLight), text(std::move(value)) | bold | color(color)}) | borderRounded | flex;
}
std::vector<Track> discover(const std::filesystem::path& root) {
  std::vector<Track> tracks;
  if (root.empty() || !std::filesystem::is_directory(root)) return tracks;
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;
    const auto ext = entry.path().extension().string();
    if (ext == ".wav" || ext == ".flac" || ext == ".mp3" || ext == ".aiff" || ext == ".aif") tracks.push_back({entry.path(), entry.path().stem().string()});
  }
  std::ranges::sort(tracks, {}, &Track::title); return tracks;
}
}  // namespace

int main(int argc, char** argv) {
  using namespace ftxui;
  std::filesystem::path root = argc > 1 ? argv[1] : std::filesystem::current_path();
  beatglass::cli::NavigationModel navigation;
  auto tracks = discover(root);
  int tab = 0, selected = tracks.empty() ? -1 : 0;
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::optional<beatglass::TempoResult> result; bool busy = false;
  Popup popup = Popup::none;
  std::string popup_message;
  std::string draft_folder = root.string();
  auto popup_title = [&] {
    switch (popup) {
      case Popup::folder: return std::string("ADD LIBRARY FOLDER");
      case Popup::output: return std::string("OUTPUT DEVICE");
      case Popup::colors: return std::string("COLORS");
      case Popup::settings: return std::string("SETTINGS");
      case Popup::permissions: return std::string("PERMISSIONS");
      case Popup::recovery: return std::string("DEVICE RECOVERY");
      case Popup::confirm_remove: return std::string("REMOVE LIBRARY FOLDER?");
      case Popup::none: return std::string();
    }
    return std::string();
  };
  auto analyze = [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size()) || busy) return;
    busy = true; status = "Analyzing " + tracks[selected].title + "…";
    try { result = beatglass::analyze_file(tracks[selected].path); status = "Analysis complete — estimates are backed by the file analyzer."; }
    catch (const std::exception& error) { result.reset(); status = std::string("Analysis failed: ") + error.what(); }
    busy = false;
  };
  auto reload_folder = [&](const std::filesystem::path& folder) {
    root = folder;
    tracks = discover(root);
    selected = tracks.empty() ? -1 : 0;
    result.reset();
    status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  };
  auto library = Renderer([&] {
    Elements rows;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
      auto row = text((i == selected ? "▸ " : "  ") + tracks[i].title);
      if (i == selected) row = row | color(Color::Cyan) | bold; rows.push_back(row);
    }
    if (rows.empty()) rows.push_back(text("No supported audio files in this folder.") | color(Color::GrayLight));
    return vbox({text("LIBRARY") | bold | color(Color::Cyan), separator(), text(root.string()) | color(Color::GrayLight), separator(), vbox(rows) | vscroll_indicator | frame | flex, separator(), text("↑↓ select   Enter analyze   Tab switch view   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto analysis = Renderer([&] {
    const auto name = selected >= 0 ? tracks[selected].title : "No track selected";
    Elements cards = {metric("TEMPO", result ? format_bpm(result->bpm) + " BPM" : "—", Color::GreenLight), metric("BEATS", result ? std::to_string(result->beats) : "—", Color::YellowLight), metric("STATE", result ? beatglass::cli::state_label(result->state) : "waiting", Color::Cyan)};
    return vbox({text("TRACK ANALYSIS") | bold | color(Color::Cyan), text(name) | color(Color::White), separator(), hbox(cards) | size(HEIGHT, LESS_THAN, 7), separator(), text(result ? "Analysis snapshot" : "Select a track in Library, then press Enter") | bold, text(result ? "Format: " + result->metadata.format : "Real decoder and aubio analysis will populate this view."), text(result ? "Sample rate: " + std::to_string(result->metadata.sample_rate) : "No placeholder data is shown."), separator(), text(status) | color(busy ? Color::YellowLight : Color::GrayLight), text("Tab switch view   Enter analyze selected   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto now_playing = Renderer([&] { return vbox({text("NOW PLAYING") | bold | color(Color::Cyan), separator(), text(result && selected >= 0 ? tracks[selected].title : "Nothing loaded") | bold, text("Playback controls are staged for the next slice."), separator(), hbox({text("  ◀  ") | borderRounded, text("  ▶  ") | borderRounded, text("  ■  ") | borderRounded}) | center, filler(), text("Tab switch view   q quit") | color(Color::GrayDark)}) | borderRounded | flex; });
  auto tabs = Container::Tab({library, analysis, now_playing}, &tab);
  auto rail = Renderer([&] {
    const auto item = [&](int index, std::string label) {
      auto row = text((tab == index ? "▸ " : "  ") + std::move(label));
      return tab == index ? row | color(Color::Cyan) | bold : row | color(Color::GrayLight);
    };
    return vbox({text("BEATGLASS") | bold | color(Color::Cyan), separator(), item(0, "Library"), item(1, "Live"), item(2, "Settings"), filler(), text("v2 workspace") | color(Color::GrayDark)}) | borderRounded;
  });
  auto workspace = Renderer([&] {
    if (tab == 0 && navigation.layout() == beatglass::cli::Layout::two_column)
      return hbox({library->Render(), analysis->Render()}) | flex;
    return tabs->Render() | flex;
  });
  auto screen = ScreenInteractive::TerminalOutput();
  auto popup_view = Renderer([&] {
    if (popup == Popup::none) return text("");
    Elements body;
    if (popup == Popup::folder) {
      body = {text("Enter a folder path, then press Enter to open it."), text(draft_folder) | border, text("Enter open   Escape cancel") | color(Color::GrayLight)};
    } else if (popup == Popup::output) {
      body = {text("No output device is currently selected."), text("Refresh devices or choose an available system output."), text("Enter refresh   Escape cancel") | color(Color::GrayLight)};
    } else if (popup == Popup::colors) {
      body = {text("Theme: Default"), text("Accent: Cyan"), text("Enter apply   Escape cancel") | color(Color::GrayLight)};
    } else if (popup == Popup::settings) {
      body = {text("Analysis: real file analyzer"), text("Reduce motion: off"), text("Audio files are never modified."), text("Enter save   Escape cancel") | color(Color::GrayLight)};
    } else if (popup == Popup::permissions) {
      body = {text("Permissions are requested only by the selected platform backend."), text("No permission request is pending."), text("Escape close") | color(Color::GrayLight)};
    } else if (popup == Popup::recovery) {
      body = {text("The selected output device is unavailable."), text("Reconnect it or choose another output, then retry."), text("Enter retry   Escape cancel") | color(Color::GrayLight)};
    } else {
      body = {text("This removes the folder from the library only."), text("User audio files will not be deleted."), text("Enter remove   Escape cancel") | color(Color::YellowLight)};
    }
    if (!popup_message.empty()) body.push_back(text(popup_message) | color(Color::YellowLight));
    return vbox({text(popup_title()) | bold | color(Color::Cyan), separator(), vbox(body), separator(), text("Escape always cancels this popup") | color(Color::GrayDark)}) | borderDouble | size(WIDTH, GREATER_THAN, 58) | center;
  });
  auto palette_view = Renderer([&] {
    if (navigation.modal() != beatglass::cli::Modal::command_palette) return text("");
    Elements commands;
    const auto visible = navigation.visible_commands();
    for (std::size_t i = 0; i < visible.size(); ++i) {
      auto row = text(visible[i].shortcut + "  " + visible[i].label);
      if (static_cast<int>(i) == navigation.command_index()) row = row | color(Color::Cyan) | bold;
      commands.push_back(row);
    }
    if (commands.empty()) commands.push_back(text("No matching commands.") | color(Color::GrayLight));
    return vbox({text("COMMAND PALETTE") | bold | color(Color::Cyan), separator(), text("Type to filter; ↑↓ move; Enter run; Escape close") | color(Color::GrayLight), vbox(commands)})
      | borderDouble | size(WIDTH, GREATER_THAN, 58) | center;
  });
  auto root_component = CatchEvent(tabs, [&](Event event) {
    if (popup != Popup::none) {
      if (event == Event::Escape) { popup = Popup::none; popup_message.clear(); return true; }
      if (event == Event::Return) {
        if (popup == Popup::folder) {
          const std::filesystem::path chosen(draft_folder);
          if (!std::filesystem::is_directory(chosen)) { popup_message = "Folder is unavailable; choose an existing directory."; return true; }
          reload_folder(chosen);
          popup = Popup::none; popup_message.clear();
        } else if (popup == Popup::confirm_remove) {
          reload_folder({});
          status = "Library folder removed. User audio files were not deleted.";
          popup = Popup::none; popup_message.clear();
        } else if (popup == Popup::recovery) {
          popup_message = "Retry requested; reconnect the device if it remains unavailable.";
        } else {
          popup = Popup::none; popup_message.clear();
        }
        return true;
      }
      if (popup == Popup::folder && event == Event::Backspace) {
        if (!draft_folder.empty()) draft_folder.pop_back();
        return true;
      }
      if (popup == Popup::folder && event.is_character()) { draft_folder += event.character(); return true; }
      return true;
    }
    if (event == Event::Character("/")) { navigation.toggle_palette(); return true; }
    if (navigation.modal() == beatglass::cli::Modal::command_palette) {
      if (event == Event::Escape) { navigation.dismiss_modal(); return true; }
      if (event == Event::ArrowDown) { navigation.move_command(1); return true; }
      if (event == Event::ArrowUp) { navigation.move_command(-1); return true; }
      if (event == Event::Backspace) { auto query = navigation.query(); if (!query.empty()) { query.pop_back(); navigation.set_query(query); } return true; }
      if (event.is_character()) { navigation.set_query(navigation.query() + event.character()); return true; }
      return true;
    }
    if (event == Event::Escape) { screen.Exit(); return true; }
    if (event == Event::Character("q")) { screen.Exit(); return true; }
    if (event == Event::Character("f")) { popup = Popup::folder; draft_folder = root.string(); return true; }
    if (event == Event::Character("o")) { popup = Popup::output; return true; }
    if (event == Event::Character("c")) { popup = Popup::colors; return true; }
    if (event == Event::Character("s")) { popup = Popup::settings; return true; }
    if (event == Event::Character("p")) { popup = Popup::permissions; return true; }
    if (event == Event::Character("r")) { popup = Popup::recovery; return true; }
    if (event == Event::Character("d")) { popup = Popup::confirm_remove; return true; }
    if (event == Event::Tab) { tab = (tab + 1) % 3; navigation.select(static_cast<beatglass::cli::Pane>(tab)); return true; }
    if (event == Event::Return && tab == 0) { analyze(); return true; }
    if (event == Event::ArrowDown && tab == 0 && !tracks.empty()) { selected = std::min(selected + 1, static_cast<int>(tracks.size()) - 1); return true; }
    if (event == Event::ArrowUp && tab == 0 && !tracks.empty()) { selected = std::max(selected - 1, 0); return true; }
    return false;
  });
  screen.Loop(Renderer(root_component, [&] { return dbox({hbox({rail->Render() | size(WIDTH, LESS_THAN, 18), workspace->Render()}), popup_view->Render(), palette_view->Render()}); })); return 0;
}
