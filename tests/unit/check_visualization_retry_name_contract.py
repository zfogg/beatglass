"""Verify capture retry status names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationRetryFeedback"',
    'Accessible.name: "Visualization capture recovery status, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: text',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing visualization retry markers: " + ", ".join(missing))
print(f"Visualization retry name contract verified ({len(required)} checks)")
