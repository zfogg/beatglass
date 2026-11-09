"""Contract check for persisted retry identity display."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
for marker in ("retryableRunDetails", "run.key.content", "run.key.engine", "run.key.config", "run.key.mode", "run.source_path", "run.decoder_snapshot"):
    if marker not in header + source:
        raise SystemExit(f"Missing retryable detail marker: {marker}")
print("Retryable lifecycle detail contract verified (5 checks)")
