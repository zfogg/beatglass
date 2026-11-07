#include "beatglass/core/file_analysis.hpp"
#include <exception>
#include <iostream>
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  const bool replay = argc == 3 && std::filesystem::path(argv[1]) == std::filesystem::path("--replay");
  if (argc != 2 && !replay) { std::cerr << "Usage: beatglass-cli [--replay] <audio-file>\n"; return 2; }
  try {
    const auto path = std::filesystem::path(argv[replay ? 2 : 1]);
    const auto r = replay ? beatglass::replay_file(path, [](std::uint64_t frame, const beatglass::TempoResult& state) {
      std::cout << "observed frame: " << frame << " tempo: ";
      if (state.bpm) std::cout << *state.bpm; else std::cout << "unknown";
      std::cout << "\n";
    }) : beatglass::analyze_file(path);
    std::cout << "format: " << r.metadata.format << "\nsample rate: " << r.metadata.sample_rate << "\nchannels: " << r.metadata.channels << "\nframes: " << r.metadata.frames << "\nbeats: " << r.beats << "\ntempo: ";
    if (r.bpm) std::cout << *r.bpm << " BPM\n"; else std::cout << "unknown\n";
  } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
}
