"""Verify analysis handoff names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationAnalysisHandoff"',
    'Accessible.name: "Visualization analysis handoff, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending: " : "overlay ready: ") + text',
    'Accessible.value: text',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing analysis handoff markers: " + ", ".join(missing))
print(f"Visualization handoff name contract verified ({len(required)} checks)")
