#include "beatglass/core/file_analysis.hpp"
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
  const std::filesystem::path root = argc > 1 ? argv[1] : std::filesystem::current_path();
  auto tracks = discover(root);
  int tab = 0, selected = tracks.empty() ? -1 : 0;
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::optional<beatglass::TempoResult> result; bool busy = false;
  auto analyze = [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size()) || busy) return;
    busy = true; status = "Analyzing " + tracks[selected].title + "…";
    try { result = beatglass::analyze_file(tracks[selected].path); status = "Analysis complete — estimates are backed by the file analyzer."; }
    catch (const std::exception& error) { result.reset(); status = std::string("Analysis failed: ") + error.what(); }
    busy = false;
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
  auto screen = ScreenInteractive::TerminalOutput();
  auto root_component = CatchEvent(tabs, [&](Event event) {
    if (event == Event::Character("q") || event == Event::Escape) { screen.Exit(); return true; }
    if (event == Event::Return && tab == 0) { analyze(); return true; }
    if (event == Event::ArrowDown && tab == 0 && !tracks.empty()) { selected = std::min(selected + 1, static_cast<int>(tracks.size()) - 1); return true; }
    if (event == Event::ArrowUp && tab == 0 && !tracks.empty()) { selected = std::max(selected - 1, 0); return true; }
    return false;
  });
  screen.Loop(root_component); return 0;
}
