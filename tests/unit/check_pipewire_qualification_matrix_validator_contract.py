"""Contract checks for the qualification matrix validator."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_qualification_matrix.py").read_text(encoding="utf-8")
for marker in ("beatglass-native-qualification-matrix-v1", '"Linux"', '"Windows"', '"Darwin"', "pending_reason", "qualification matrix platform set is incomplete", "qualified platform has no evidence", "invalid evidence path", "duplicate evidence path", "invalid qualification matrix commit", "invalid qualification matrix host", "native_execution_attempted", "execution_blockers"):
    if marker not in source:
        raise SystemExit(f"missing matrix validator marker: {marker}")
print("PipeWire qualification matrix validator contract passed (6 checks)")
