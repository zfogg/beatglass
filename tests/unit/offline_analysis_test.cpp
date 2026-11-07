#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/offline_analysis.hpp"
#include <limits>
using namespace beatglass;
namespace {
OfflineAnalysis example() {
 OfflineAnalysis r; r.epoch = 7; r.sample_rate = 48000; r.observed_through = 110; r.analyzed_through = 100;
 r.engine_version = "engine-1"; r.configuration_id = "profile-hop-downmix-1"; r.algorithms = {{"tempo", "1"}, {"tonal", "2"}};
 r.beats = {{9, 16, std::nullopt}, {49, 60, 0.3f}};
 r.tonal_windows = {{10, 40, 0, OfflineEstimate{"C", std::nullopt}, OfflineEstimate{"C", 0.5f}},
                    {30, 70, 5, OfflineEstimate{"Am", 0.2f}, OfflineEstimate{"Am", std::nullopt}},
                    {80, 100, std::nullopt, std::nullopt, std::nullopt}};
 return r;
}
constexpr OfflineLimits limits{10, 10, 10, 128};
}
TEST_CASE("offline results preserve unknown beat strength and overlapping evidence") {
 auto r = example(); REQUIRE(validate_offline_analysis(r, limits) == OfflineValidation::valid);
 CHECK_FALSE(r.beats[0].strength); CHECK_FALSE(r.validated_track_key);
 CHECK(r.tonal_windows[0].end_frame > r.tonal_windows[1].start_frame);
 CHECK(*r.tonal_windows[1].rolling_key_from < r.tonal_windows[1].start_frame);
 CHECK_FALSE(r.tonal_windows[2].local_chord); CHECK(r.tonal_windows[1].end_frame < r.tonal_windows[2].start_frame);
 auto owned = r; r.tonal_windows[0].rolling_key->label = "changed";
 CHECK(owned.tonal_windows[0].rolling_key->label == "C");
}
TEST_CASE("offline result validation enforces exclusive boundaries and chronological order") {
 auto r = example(); r.beats[0].frame = r.beats[0].observed_through;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_beat);
 r = example(); r.beats[1].observed_through = 101;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_beat);
 r = example(); r.beats[1].frame = 8;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_beat);
 r = example(); r.beats = {{9, 70, std::nullopt}, {49, 60, std::nullopt}};
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_beat);
 r.beats[1].observed_through = 70;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::valid);
 r = example(); r.tonal_windows[1].end_frame = r.tonal_windows[1].start_frame;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_window);
 r = example(); r.tonal_windows[1].end_frame = 101;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_window);
 r = example(); r.tonal_windows[1].end_frame = 39;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_window);
 r = example(); r.tonal_windows[0].rolling_key_from.reset();
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_window);
}
TEST_CASE("offline results bound storage and distinguish absent from invalid estimates") {
 auto r = example(); CHECK(validate_offline_analysis(r, {1, 10, 10, 128}) == OfflineValidation::limit_exceeded);
 CHECK(validate_offline_analysis(r, {10, 2, 10, 128}) == OfflineValidation::limit_exceeded);
 CHECK(validate_offline_analysis(r, {10, 10, 1, 128}) == OfflineValidation::limit_exceeded);
 CHECK(validate_offline_analysis(r, {10, 10, 10, 3}) == OfflineValidation::limit_exceeded);
 r.validated_track_key = OfflineEstimate{"C major", std::nullopt};
 REQUIRE(validate_offline_analysis(r, limits) == OfflineValidation::valid);
 r.validated_track_key->strength = std::numeric_limits<float>::quiet_NaN();
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_metadata);
 r = example(); r.analyzed_through = 111;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_metadata);
 r = example(); r.epoch = 0;
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_metadata);
 r = example(); r.algorithms.clear();
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_metadata);
 r = example(); r.tonal_windows[0].local_chord->label.clear();
 CHECK(validate_offline_analysis(r, limits) == OfflineValidation::invalid_window);
}
