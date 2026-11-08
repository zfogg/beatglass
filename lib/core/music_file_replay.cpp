#include "beatglass/core/music_replay.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"
#include <vector>
namespace beatglass {
CausalMusicState replay_music_file(const std::filesystem::path& path, std::uint64_t epoch,
                                  const CausalMusicReplay::BeatObserver& observer, std::stop_token cancel,
                                  const CausalMusicReplay::TonalObserver& tonal_observer,
                                  const CausalMusicReplay::StateObserver& state_observer) {
  if (!epoch) throw std::invalid_argument("Music replay requires a nonzero epoch");
  if (cancel.stop_requested()) throw MusicReplayCancelled();
  const auto initial_fingerprint = library_detail::fingerprint_file(path, cancel);
  if (initial_fingerprint.status != library_detail::FingerprintStatus::ok) {
    if (initial_fingerprint.status == library_detail::FingerprintStatus::cancelled) throw MusicReplayCancelled();
    throw std::runtime_error("Replay source fingerprint failed: " + initial_fingerprint.diagnostic);
  }
  FileReader file(path, epoch);
  CausalMusicReplay replay;
  std::vector<float> samples(512 * static_cast<std::size_t>(file.metadata().channels));
  CausalMusicState state; state.sample_rate = file.metadata().sample_rate; state.tempo.epoch = epoch;
  while (true) {
    const auto read = file.read(samples, cancel);
    if (read.status == FileReadStatus::cancelled) throw MusicReplayCancelled();
    if (read.status == FileReadStatus::end) break;
    state = replay.process(read.block, observer, tonal_observer);
    if (state_observer) state_observer(state);
  }
  const auto final_fingerprint = library_detail::fingerprint_file(path, cancel);
  if (final_fingerprint.status != library_detail::FingerprintStatus::ok || final_fingerprint.digest != initial_fingerprint.digest || final_fingerprint.size != initial_fingerprint.size) {
    if (final_fingerprint.status == library_detail::FingerprintStatus::cancelled) throw MusicReplayCancelled();
    throw std::runtime_error("Replay source changed during analysis");
  }
  return state;
}
}
