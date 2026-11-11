#include "beatglass/core/file_analysis.hpp"
#include "state_label.hpp"
#include <exception>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string_view>

namespace {
void usage() { std::cout << "Usage: beatglass-cli [--help] [--version] [--replay] [--json] [--low-color <#rrggbb>] [--mid-color <#rrggbb>] [--high-color <#rrggbb>] [--] <audio-file>\n"; }
bool color_value(std::string_view value) {
  if (value.size() != 7 || value.front() != '#') return false;
  for (std::size_t i = 1; i < value.size(); ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}
void json_string(std::string_view s) {
  constexpr char hex[] = "0123456789abcdef";
  std::cout << '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') std::cout << '\\' << static_cast<char>(c);
    else if (c < 0x20) std::cout << "\\u00" << hex[c >> 4] << hex[c & 15];
    else std::cout << static_cast<char>(c);
  }
  std::cout << '"';
}
void json_result(const char* type, const beatglass::TempoResult& r, std::string_view low, std::string_view mid, std::string_view high, std::uint64_t frame = 0) {
  std::cout << "{\"type\":\"" << type << "\",\"schema_version\":1";
  if (std::string_view(type) == "replay") std::cout << ",\"observed_through\":" << frame;
  std::cout << ",\"format\":"; json_string(r.metadata.format);
  std::cout << ",\"sample_rate\":" << r.metadata.sample_rate << ",\"channels\":" << r.metadata.channels
            << ",\"frames\":" << r.metadata.frames << ",\"duration_seconds\":"
            << (r.metadata.sample_rate ? static_cast<double>(r.metadata.frames) / r.metadata.sample_rate : 0.0)
            << ",\"beats\":" << r.beats << ",\"bpm\":";
  if (r.bpm) std::cout << *r.bpm; else std::cout << "null";
  std::cout << ",\"state\":\"" << beatglass::cli::state_label(r.state) << "\",\"visualization_colors\":{";
  std::cout << "\"low\":"; json_string(low); std::cout << ",\"mid\":"; json_string(mid); std::cout << ",\"high\":"; json_string(high); std::cout << "}}\n";
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(9);
  bool replay = false, json = false, positional_only = false;
  std::string low_color = "#38bdf8", mid_color = "#a78bfa", high_color = "#f472b6";
  std::filesystem::path path;
  for (int i = 1; i < argc; ++i) {
    const std::filesystem::path arg(argv[i]);
    if (!positional_only && arg == std::filesystem::path("--")) positional_only = true;
    else if (!positional_only && (arg == std::filesystem::path("--help") || arg == std::filesystem::path("-h"))) { usage(); return 0; }
    else if (!positional_only && (arg == std::filesystem::path("--version") || arg == std::filesystem::path("-V"))) { std::cout << "beatglass-cli " << BEATGLASS_VERSION << "\n"; return 0; }
    else if (!positional_only && arg == std::filesystem::path("--replay") && !replay) replay = true;
    else if (!positional_only && arg == std::filesystem::path("--json") && !json) json = true;
    else if (!positional_only && (arg == std::filesystem::path("--low-color") || arg == std::filesystem::path("--mid-color") || arg == std::filesystem::path("--high-color"))) {
      if (i + 1 >= argc) { usage(); return 2; }
      const std::string value = std::filesystem::path(argv[++i]).string();
      if (!color_value(value)) { usage(); return 2; }
      if (arg == std::filesystem::path("--low-color")) low_color = value;
      else if (arg == std::filesystem::path("--mid-color")) mid_color = value;
      else high_color = value;
    }
    else if (path.empty() && arg.native().size() && (positional_only || arg.native()[0] != '-')) path = arg;
    else { usage(); return 2; }
  }
  if (path.empty()) { usage(); return 2; }
  try {
    const auto r = replay ? beatglass::replay_file(path, [json, &low_color, &mid_color, &high_color](std::uint64_t frame, const beatglass::TempoResult& state) {
      if (json) { json_result("replay", state, low_color, mid_color, high_color, frame); return; }
      const auto duration = state.metadata.sample_rate
          ? static_cast<double>(state.metadata.frames) / state.metadata.sample_rate : 0.0;
      std::cout << "observed frame: " << frame << " duration seconds: " << duration << " tempo: ";
      if (state.bpm) std::cout << *state.bpm; else std::cout << "unknown";
      std::cout << " state: " << beatglass::cli::state_label(state.state) << "\n";
    }, [json](const beatglass::ReplayBeat& beat) {
      if (!json) return;
      std::cout << "{\"type\":\"beat\",\"schema_version\":1,\"epoch\":" << beat.epoch
                << ",\"frame\":" << beat.frame << ",\"observed_through\":" << beat.observed_through << "}\n";
    }) : beatglass::analyze_file(path);
    if (json) { json_result("result", r, low_color, mid_color, high_color); return 0; }
    std::cout << "format: " << r.metadata.format << "\nsample rate: " << r.metadata.sample_rate << "\nchannels: " << r.metadata.channels << "\nframes: " << r.metadata.frames << "\nduration seconds: " << (r.metadata.sample_rate ? static_cast<double>(r.metadata.frames) / r.metadata.sample_rate : 0.0) << "\nbeats: " << r.beats << "\ntempo: ";
    if (r.bpm) std::cout << *r.bpm << " BPM\n"; else std::cout << "unknown\n";
    std::cout << "visualization colors: low " << low_color << ", mid " << mid_color << ", high " << high_color << "\n";
  } catch (const std::exception& e) { std::cerr << "error: " << e.what() << "\n"; return 1; }
  return 0;
}
