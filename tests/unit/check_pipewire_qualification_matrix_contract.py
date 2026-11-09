"""Check the cross-platform qualification matrix scaffold."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "qualification_matrix.py").read_text(encoding="utf-8")
for marker in ("beatglass-native-qualification-matrix-v1", '"Linux"', '"Windows"', '"Darwin"', "pending_reason", "native host execution pending"):
    if marker not in source:
        raise SystemExit(f"missing qualification matrix marker: {marker}")
print("PipeWire qualification matrix contract passed (6 checks)")
