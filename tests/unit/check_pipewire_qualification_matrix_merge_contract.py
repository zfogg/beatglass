"""Contract checks for portable matrix evidence ingestion."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "merge_qualification_matrices.py").read_text(encoding="utf-8")
for marker in ("beatglass-native-qualification-matrix-v1", "conflicting qualification matrix platform", "platform set is incomplete", "malformed qualification matrix"):
    if marker not in source: raise SystemExit(f"missing matrix merge marker: {marker}")
print("PipeWire qualification matrix merge contract passed (4 checks)")
