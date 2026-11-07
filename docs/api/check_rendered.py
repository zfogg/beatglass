"""Smoke-check representative declarations in Hawkmoth's HTML output."""
from pathlib import Path
import sys

html = Path(sys.argv[1]).read_text(encoding="utf-8")
expected = ("AudioBlock", "AudioClockAnchor", "frame_to_monotonic_ns", "reduce_loudest_channel", "StreamContinuity", "AudioQueue", "AudioSource", "SourceStatus", "ChannelLayout",
            "TempoEstimator", "KeyEstimator", "ChordEstimator", "PresentationAdapter",
            "AnalysisStore", "replay_file", "Metadata")
missing = [name for name in expected if name not in html]
if missing:
    raise SystemExit(f"Missing rendered declarations: {missing}")
print(f"Rendered declaration smoke check passed ({len(expected)} names)")
