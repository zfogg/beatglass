"""Contract checks for portable qualification bundle validation."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_qualification_bundle.py").read_text(encoding="utf-8")
for marker in ("qualification bundle", "Native qualification matrix", "missing platform", "beatglass-native-qualification-matrix-v1"):
    if marker not in source: raise SystemExit(f"missing qualification bundle marker: {marker}")
print("PipeWire qualification bundle contract passed (4 checks)")
