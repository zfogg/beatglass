"""Smoke-check representative declarations in Hawkmoth's HTML output."""
from pathlib import Path
import sys

html = Path(sys.argv[1]).read_text(encoding="utf-8")
expected = ("AudioBlock", "AudioClockAnchor", "frame_to_monotonic_ns", "reduce_loudest_channel", "StreamContinuity", "AudioQueue", "AudioSource", 'id="beatglass::AudioSource::stop"', "SourceStatus", "ChannelLayout",
            'id="beatglass::AcquisitionToken"', 'id="beatglass::AcquisitionToken::epoch__std::uint64_t"',
            'id="beatglass::AcquisitionToken::generation__std::uint64_t"',
            'id="beatglass::TrackingMachine"', 'id="beatglass::TrackingMachine::state__voidC"',
            'id="beatglass::TrackingMachine::token__voidC"', 'id="beatglass::TrackingMachine::start__std::uint64_t"',
            'id="beatglass::TrackingMachine::stop__void"', 'id="beatglass::TrackingMachine::fail__void"',
            'id="beatglass::TrackingMachine::discontinuity__std::uint64_t"',
            'id="beatglass::TrackingMachine::silence__void"', 'id="beatglass::TrackingMachine::signal__void"',
            'id="beatglass::TrackingMachine::expire__void"', 'id="beatglass::TrackingMachine::accept__AcquisitionToken"',
            'id="beatglass::TempoResult::state__TrackingState"', "TempoEstimator", "KeyEstimator", "ChordEstimator", "PresentationAdapter",
            "AnalysisStore", "replay_file", "Metadata", 'id="beatglass::TempoEstimator::take_beats__void"',
            'id="beatglass::ChordEstimator::take_segments__void"',
            'id="beatglass::PresentationAdapter::present__SnapshotCRC"')
missing = [name for name in expected if name not in html]
if missing:
    raise SystemExit(f"Missing rendered declarations: {missing}")
print(f"Rendered declaration smoke check passed ({len(expected)} names)")
