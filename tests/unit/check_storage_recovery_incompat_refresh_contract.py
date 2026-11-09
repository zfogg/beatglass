from pathlib import Path

source = Path("src/desktop/storage_recovery_bridge.cpp").read_text()
if "incompatible_decoder_snapshot))); refresh(); return;" not in source:
    raise SystemExit("incompatible decoder retry rejection must refresh recovery state")
print("storage recovery incompatible refresh contract verified")
