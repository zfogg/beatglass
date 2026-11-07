"""Smoke-check representative declarations in Hawkmoth's HTML output."""
from pathlib import Path
import sys

html = Path(sys.argv[1]).read_text(encoding="utf-8")
expected = ("AudioBlock", "AudioClockAnchor", "frame_to_monotonic_ns", "reduce_loudest_channel", "StreamContinuity", "AudioQueue", "AudioSource", 'id="beatglass::AudioSource::stop"', "SourceStatus", "ChannelLayout", "SourceAttempt", "SourceLifecycle",
            'id="beatglass::AcquisitionToken"', 'id="beatglass::AcquisitionToken::epoch__std::uint64_t"',
            'id="beatglass::AcquisitionToken::generation__std::uint64_t"',
            'id="beatglass::TrackingMachine"', 'id="beatglass::TrackingMachine::state__voidC"',
            'id="beatglass::TrackingMachine::token__voidC"', 'id="beatglass::TrackingMachine::start__std::uint64_t"',
            'id="beatglass::TrackingMachine::stop__void"', 'id="beatglass::TrackingMachine::fail__void"',
            'id="beatglass::TrackingMachine::discontinuity__std::uint64_t"',
            'id="beatglass::TrackingMachine::silence__void"', 'id="beatglass::TrackingMachine::signal__void"',
            'id="beatglass::TrackingMachine::expire__void"', 'id="beatglass::TrackingMachine::accept__AcquisitionToken"',
            'id="beatglass::SnapshotPublisher"', 'id="beatglass::SnapshotPublisher::SnapshotPublisher__std::s.std::s"',
            'id="beatglass::SnapshotPublisher::begin_acquisition__AcquisitionToken"',
            'id="beatglass::SnapshotPublisher::clear__void"',
            'id="beatglass::SnapshotPublisher::publish__AcquisitionToken.Snapshot"',
            'id="beatglass::SnapshotPublisher::latest__voidC"',
            'id="beatglass::FileMetadata"', 'id="beatglass::FileMetadata::sample_rate__std::uint32_t"',
            'id="beatglass::FileMetadata::channels__std::uint32_t"', 'id="beatglass::FileMetadata::frames__std::uint64_t"',
            'id="beatglass::FileMetadata::format__ss"', 'id="beatglass::TempoResult"',
            'id="beatglass::TempoResult::metadata__FileMetadata"', 'id="beatglass::TempoResult::bpm__std::optional:float:"',
            'id="beatglass::TempoResult::beats__unsigned-i"', 'id="beatglass::TempoResult::state__TrackingState"', "TempoEstimator", "KeyEstimator", "ChordEstimator", "PresentationAdapter",
            'id="_CPPv4N9beatglass14FileReadStatusE"', 'id="_CPPv4N9beatglass14FileReadStatus4dataE"',
            'id="_CPPv4N9beatglass14FileReadStatus3endE"', 'id="_CPPv4N9beatglass14FileReadStatus9cancelledE"',
            'id="beatglass::FileReadResult"', 'id="beatglass::FileReadResult::status__FileReadStatus"',
            'id="beatglass::FileReadResult::block__AudioBlock"', 'id="beatglass::FileReader"',
            'id="beatglass::FileReader::FileReader__std::filesystem::pathCR.std::uint64_t"',
            'id="beatglass::FileReader::~FileReader__void"', 'id="beatglass::FileReader::FileReader__FileReaderCR"',
            'id="beatglass::FileReader::assign-operator__FileReaderCR"', 'id="beatglass::FileReader::metadata__voidC"',
            'id="beatglass::FileReader::read__std::span:float:.std::stop_token"',
            'id="beatglass::FileReader::seek__std::uint64_t"', "AnalysisStore", "replay_file", "Metadata", 'id="beatglass::TempoEstimator::take_beats__void"',
            'id="beatglass::ChordEstimator::take_segments__void"',
            'id="beatglass::PresentationAdapter::present__SnapshotCRC"')
missing = [name for name in expected if name not in html]
if missing:
    raise SystemExit(f"Missing rendered declarations: {missing}")
print(f"Rendered declaration smoke check passed ({len(expected)} names)")
