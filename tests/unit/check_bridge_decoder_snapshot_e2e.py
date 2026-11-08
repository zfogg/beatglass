"""Bridge decision contract for compatible and incompatible persisted retries."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
storage = (root / "tests/unit/library_sqlite_cache_test.cpp").read_text(encoding="utf-8")
compatible = "found->decoder_snapshot != beatglass::library::kFileDecoderSnapshot"
required = [
    compatible,
    "Retry rejected: incompatible decoder snapshot",
    "make_file_decode_operation(found->source_path, std::move(identity), found->decoder_snapshot)",
    "coordinator_.resubmit_retryable(runId.toStdString(), std::move(operation))",
    'cache.begin("snapshot", key, "/music/example.flac", "decoder-v3;pcm-f32;44100")',
    'cache.begin("legacy-snapshot", key, "/music/legacy.flac")',
]
missing = [marker for marker in required if marker not in bridge + storage]
if missing:
    raise SystemExit("Missing bridge decoder snapshot E2E markers: " + ", ".join(missing))
if bridge.index(compatible) > bridge.index("make_file_decode_operation(found->source_path"):
    raise SystemExit("Bridge reconstructs an operation before validating decoder compatibility")
print(f"Bridge decoder snapshot E2E contract verified ({len(required) + 1} checks)")
