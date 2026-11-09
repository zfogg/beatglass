"""Regression contract for rejecting persisted decoder snapshots we cannot reproduce."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
cache_test = (root / "tests/unit/library_sqlite_cache_test.cpp").read_text(encoding="utf-8")
required = [
    "file_decoder_snapshot_compatible(found->decoder_snapshot)",
    "Retry rejected: incompatible decoder snapshot",
    '"decoder-v3;pcm-f32;44100"',
    "runs.front().decoder_snapshot ==",
    'cache.begin("legacy-snapshot", key, "/music/legacy.flac")',
    "runs.front().decoder_snapshot.empty()",
    "Retry rejected: incompatible decoder snapshot",
]
missing = [marker for marker in required if marker not in bridge + cache_test]
if missing:
    raise SystemExit("Missing decoder snapshot mismatch regression markers: " + ", ".join(missing))
print(f"Decoder snapshot retry contract verified ({len(required)} checks)")
