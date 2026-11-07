#include "beatglass/core/file_analysis.hpp"
#include <exception>
#include <iostream>
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  if (argc != 2) { std::cerr << "Usage: beatglass-cli <audio-file>\n"; return 2; }
  try { const auto r = beatglass::analyze_file(argv[1]);
    std::cout << "format: " << r.metadata.format << "\nsample rate: " << r.metadata.sample_rate << "\nchannels: " << r.metadata.channels << "\nframes: " << r.metadata.frames << "\nbeats: " << r.beats << "\ntempo: ";
    if (r.bpm) std::cout << *r.bpm << " BPM\n"; else std::cout << "unknown\n";
  } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
}
