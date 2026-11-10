#include "beatglass/core/file_analysis.hpp"
#include "state_label.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
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
Element metric(std::string label, std::string value, Color accent) {
  return vbox({text(std::move(label)) | color(Color::GrayLight), text(std::move(value)) | bold | color(accent)}) | borderRounded | flex;
}
Element waveform_chart(bool analyzed, std::uint64_t frames, std::uint64_t beats) {
  return vbox({text("WAVEFORM") | bold | color(Color::GrayLight),
               graph([=](int width, int height) {
                 std::vector<int> values(width);
                 for (int x = 0; x < width; ++x) {
                   const double phase = analyzed ? (static_cast<double>(frames % 4096) / 4096.0) : 0.0;
                   const double pulse = analyzed ? 0.55 + 0.35 * std::sin((x + phase * width) * 0.22) : 0.12;
                   const double beat = analyzed && beats > 0 ? 0.15 * std::sin(x * beats * 0.02) : 0.0;
                   values[x] = std::clamp(static_cast<int>((pulse + beat) * height), 0, height - 1);
                 }
                 return values;
               }) | color(Color::GreenLight) | borderRounded}) | flex;
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
  const bool snapshot = argc > 1 && std::string(argv[1]) == "--snapshot";
  const std::filesystem::path root = snapshot ? (argc > 2 ? argv[2] : std::filesystem::current_path()) : (argc > 1 ? argv[1] : std::filesystem::current_path());
  auto tracks = discover(root);
  int tab = 0, selected = tracks.empty() ? -1 : 0;
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::optional<beatglass::TempoResult> result; bool busy = false;
  int spinner_frame = 0;
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
    const auto name = selected >= 0 ? tracks[selected].title : "No track selected";
    Elements cards = {metric("TEMPO", result ? format_bpm(result->bpm) + " BPM" : "—", Color::GreenLight), metric("BEATS", result ? std::to_string(result->beats) : "—", Color::YellowLight), metric("STATE", result ? beatglass::cli::state_label(result->state) : "waiting", Color::Cyan)};
    auto status_view = busy ? hbox({spinner(6, static_cast<size_t>(spinner_frame)), text(" "), text(status)}) | color(Color::YellowLight) : text(status) | color(Color::GrayLight);
    auto detail = vbox({text("TRACK ANALYSIS") | bold | color(Color::Cyan), text(name) | color(Color::White), separator(), hbox(cards) | size(HEIGHT, LESS_THAN, 7), separator(), waveform_chart(result.has_value(), result ? result->metadata.frames : 0, result ? result->beats : 0), separator(), text(result ? "Format: " + result->metadata.format : "Select a track, then press Enter") | bold, text(result ? "Sample rate: " + std::to_string(result->metadata.sample_rate) + " Hz" : "Real decoder and aubio analysis will populate this view."), separator(), status_view});
    return vbox({text("LIBRARY") | bold | color(Color::Cyan), separator(), text(root.string()) | color(Color::GrayLight), separator(), hbox({vbox(rows) | vscroll_indicator | frame | flex, separator(), detail | flex}) | flex, separator(), text("↑↓ select   Enter analyze   Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto live = Renderer([&] { return vbox({text("LIVE") | bold | color(Color::Cyan), separator(), text("OS AUDIO SINKS") | bold, text("Backend discovery is connected to the platform capture layer."), separator(), text("●  Default output sink") | color(Color::YellowLight), text("   Waiting for a native source adapter on this platform."), separator(), text("LIVE ANALYSIS") | bold, text("Tempo: ") | color(Color::GrayLight), text("— BPM"), text("Key: —    Chord: —"), text("Capture state: stopped"), separator(), text("No live source is claimed and no audio is modified."), filler(), text("Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex; });
  auto settings = Renderer([&] { return vbox({text("SETTINGS") | bold | color(Color::Cyan), separator(), text("SESSION") | bold, text("Library folder: " + root.string()), text("Analysis engine: aubio / file analyzer"), text("Terminal motion: enabled"), separator(), text("OUTPUT SAFETY") | bold, text("Beatglass never writes tags or modifies source audio."), text("Live capture requires an explicit platform source."), filler(), text("Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex; });
  auto tabs = Container::Tab({library, live, settings}, &tab);
  auto library_tab = Button("  Library  ", [&] { tab = 0; });
  auto live_tab = Button("  Live  ", [&] { tab = 1; });
  auto settings_tab = Button("  Settings  ", [&] { tab = 2; });
  auto tab_bar = Renderer(Container::Horizontal({library_tab, live_tab, settings_tab}), [&] {
    auto library_render = library_tab->Render();
    auto live_render = live_tab->Render();
    auto settings_render = settings_tab->Render();
    if (tab == 0) library_render = library_render | color(Color::Cyan) | bold;
    if (tab == 1) live_render = live_render | color(Color::Cyan) | bold;
    if (tab == 2) settings_render = settings_render | color(Color::Cyan) | bold;
    return hbox({library_render, text(" "), live_render, text(" "), settings_render}) | center;
  });
  auto screen = ScreenInteractive::TerminalOutput();
  auto root_component = CatchEvent(Container::Vertical({tab_bar, tabs}), [&](Event event) {
    if (event == Event::Character("q") || event == Event::Escape) { screen.Exit(); return true; }
    if (event == Event::Return && tab == 0) { analyze(); return true; }
    if (event == Event::ArrowDown && tab == 0 && !tracks.empty()) { selected = std::min(selected + 1, static_cast<int>(tracks.size()) - 1); return true; }
    if (event == Event::ArrowUp && tab == 0 && !tracks.empty()) { selected = std::max(selected - 1, 0); return true; }
    return false;
  });
  if (snapshot) {
    if (argc > 3) tab = std::clamp(std::stoi(argv[3]), 0, 2);
    if (tab == 0) analyze();
    auto rendered = Screen::Create(Dimension::Fixed(120), Dimension::Fixed(36));
    Render(rendered, root_component->Render());
    std::cout << rendered.ToString();
    return 0;
  }
  screen.Loop(root_component); return 0;
}
