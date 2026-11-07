#include "beatglass/core/music_replay.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <vector>
namespace beatglass {
CausalMusicState replay_music_file(const std::filesystem::path& path, std::uint64_t epoch,
                                  const CausalMusicReplay::BeatObserver& observer, std::stop_token cancel,
                                  const CausalMusicReplay::TonalObserver& tonal_observer) {
  if (cancel.stop_requested()) throw MusicReplayCancelled();
  FileReader file(path, epoch);
  CausalMusicReplay replay;
  std::vector<float> samples(512 * static_cast<std::size_t>(file.metadata().channels));
  CausalMusicState state; state.sample_rate = file.metadata().sample_rate; state.tempo.epoch = epoch;
  while (true) {
    const auto read = file.read(samples, cancel);
    if (read.status == FileReadStatus::cancelled) throw MusicReplayCancelled();
    if (read.status == FileReadStatus::end) break;
    state = replay.process(read.block, observer, tonal_observer);
  }
  return state;
}
}
