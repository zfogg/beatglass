#include "beatglass/core/music_replay.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include "state_label.hpp"
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
void optional_number(const std::optional<float>& value) { if(value) std::cout << *value; else std::cout << "null"; }
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
  if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) { std::cout << "Usage: beatglass-music-cli [--help] [--version] <audio-file>\n"; return 0; }
  if (argc == 2 && (std::string_view(argv[1]) == "--version" || std::string_view(argv[1]) == "-V")) { std::cout << "beatglass-music-cli " << BEATGLASS_VERSION << "\n"; return 0; }
#endif
  if (argc != 2) { std::cerr << "Usage: beatglass-music-cli <audio-file>\n"; return 2; }
  std::cout.imbue(std::locale::classic()); std::cout << std::setprecision(9);
  try {
    beatglass::SourceEpochAllocator epochs;
    const auto epoch=epochs.next();
    if (!epoch) throw std::runtime_error("source epoch exhausted");
    auto previous_state=beatglass::TrackingState::stopped;
    const auto result=beatglass::replay_music_file(std::filesystem::path(argv[1]),*epoch,[](const beatglass::CausalMusicBeat& beat) {
      std::cout << "{\"type\":\"beat\",\"schema_version\":1,\"epoch\":" << beat.epoch
                << ",\"sample_rate\":" << beat.sample_rate << ",\"frame\":" << beat.frame << ",\"observed_through\":" << beat.observed_through << ",\"strength\":";
      optional_number(beat.strength); std::cout << "}\n";
    }, {}, [](const beatglass::CausalTonalFrame& tonal) {
      std::cout << "{\"type\":\"tonal\",\"schema_version\":1,\"epoch\":" << tonal.epoch
                << ",\"sample_rate\":" << tonal.sample_rate << ",\"first_frame\":" << tonal.first_frame << ",\"observed_through\":" << tonal.observed_through << ",\"rolling_key\":";
      optional_string(tonal.rolling_key); std::cout << ",\"rolling_key_from\":"; if(tonal.rolling_key_from) std::cout << *tonal.rolling_key_from; else std::cout << "null";
      std::cout << ",\"local_chord\":"; optional_string(tonal.local_chord);
      std::cout << ",\"rolling_key_strength\":"; optional_number(tonal.rolling_key_strength);
      std::cout << ",\"local_chord_strength\":"; optional_number(tonal.local_chord_strength);
      std::cout << "}\n";
    }, [&](const beatglass::CausalMusicState& state) {
      if (state.tempo.state == previous_state) return;
      previous_state = state.tempo.state;
      std::cout << "{\"type\":\"state\",\"schema_version\":1,\"epoch\":" << state.tempo.epoch
                << ",\"sample_rate\":" << state.sample_rate << ",\"observed_through\":" << state.tempo.observed_through << ",\"state\":";
      string_json(beatglass::cli::state_label(state.tempo.state)); std::cout << "}\n";
    });
    std::cout << "{\"type\":\"result\",\"schema_version\":1,\"epoch\":" << result.tempo.epoch
              << ",\"sample_rate\":" << result.sample_rate << ",\"observed_through\":" << result.tempo.observed_through << ",\"analyzed_through\":" << result.tempo.analyzed_through << ",\"bpm\":";
    if(result.tempo.bpm) std::cout << *result.tempo.bpm; else std::cout << "null";
    std::cout << ",\"state\":"; string_json(beatglass::cli::state_label(result.tempo.state));
    std::cout << ",\"rolling_key\":"; optional_string(result.rolling_key); std::cout << ",\"local_chord\":"; optional_string(result.local_chord);
    std::cout << ",\"rolling_key_strength\":"; optional_number(result.rolling_key_strength);
    std::cout << ",\"local_chord_strength\":"; optional_number(result.local_chord_strength);
    std::cout << ",\"rolling_key_from\":"; if(result.rolling_key_from) std::cout << *result.rolling_key_from; else std::cout << "null";
    std::cout << ",\"tonal_from\":"; if(result.tonal_from) std::cout << *result.tonal_from; else std::cout << "null";
    std::cout << ",\"tonal_through\":"; if(result.tonal_through) std::cout << *result.tonal_through; else std::cout << "null";
    std::cout << "}\n";
  } catch(const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
