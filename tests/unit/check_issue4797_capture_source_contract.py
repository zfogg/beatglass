"""Validate GUI/CLI capture-source attribution markers."""
from pathlib import Path
root = Path(__file__).parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
cli = (root / "src/cli/main.cpp").read_text(encoding="utf-8")
for marker in ('captureSourceMode', 'Microphone', 'Room audio', 'System output', 'discontinuity'):
    if marker not in qml:
        raise SystemExit(f"missing GUI capture source marker: {marker}")
for marker in ('--source-mode', 'capture_source', 'microphone', 'room-audio'):
    if marker not in cli:
        raise SystemExit(f"missing CLI capture source marker: {marker}")
print("issue #4797 capture source contract: PASS")
