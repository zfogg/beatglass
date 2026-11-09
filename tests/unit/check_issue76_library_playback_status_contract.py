"""Verify playback/source alignment is visible from the Library surface."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "libraryPlaybackRecoveryStatus"',
    'window.playbackBridge.state',
    'window.playbackMatchesSelection',
    'Reopen selected playback to align the source before seeking.',
    'objectName: "reopenSelectedPlayback"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise AssertionError("missing Library playback status markers: " + ", ".join(missing))
print(f"Library playback status contract: {len(required)} checks passed")
