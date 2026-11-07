#include "beatglass/core/music_replay.hpp"
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string_view>
namespace {
void string_json(std::string_view value) {
  constexpr char hex[]="0123456789abcdef";
  std::cout << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\') std::cout << '\\' << static_cast<char>(c);
    else if (c < 0x20) std::cout << "\\u00" << hex[c>>4] << hex[c&15];
    else std::cout << static_cast<char>(c);
  }
  std::cout << '"';
}
void optional_string(const std::optional<std::string>& value) { if(value) string_json(*value); else std::cout << "null"; }
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  if (argc != 2) { std::cerr << "Usage: beatglass-music-cli <audio-file>\n"; return 2; }
  std::cout.imbue(std::locale::classic()); std::cout << std::setprecision(9);
  try {
    const auto result=beatglass::replay_music_file(std::filesystem::path(argv[1]),0,[](const beatglass::CausalMusicBeat& beat) {
      std::cout << "{\"type\":\"beat\",\"schema_version\":1,\"epoch\":" << beat.epoch
                << ",\"frame\":" << beat.frame << ",\"observed_through\":" << beat.observed_through << "}\n";
    }, {}, [](const beatglass::CausalTonalFrame& tonal) {
      std::cout << "{\"type\":\"tonal\",\"schema_version\":1,\"epoch\":" << tonal.epoch
                << ",\"first_frame\":" << tonal.first_frame << ",\"observed_through\":" << tonal.observed_through << ",\"key\":";
      optional_string(tonal.key); std::cout << ",\"key_from\":"; if(tonal.key_from) std::cout << *tonal.key_from; else std::cout << "null";
      std::cout << ",\"chord\":"; optional_string(tonal.chord);
      std::cout << "}\n";
    });
    std::cout << "{\"type\":\"result\",\"schema_version\":1,\"epoch\":" << result.tempo.epoch
              << ",\"observed_through\":" << result.tempo.observed_through << ",\"analyzed_through\":" << result.tempo.analyzed_through << ",\"bpm\":";
    if(result.tempo.bpm) std::cout << *result.tempo.bpm; else std::cout << "null";
    std::cout << ",\"key\":"; optional_string(result.key); std::cout << ",\"chord\":"; optional_string(result.chord);
    std::cout << ",\"key_from\":"; if(result.key_from) std::cout << *result.key_from; else std::cout << "null";
    std::cout << ",\"tonal_from\":"; if(result.tonal_from) std::cout << *result.tonal_from; else std::cout << "null";
    std::cout << ",\"tonal_through\":"; if(result.tonal_through) std::cout << *result.tonal_through; else std::cout << "null";
    std::cout << "}\n";
  } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
