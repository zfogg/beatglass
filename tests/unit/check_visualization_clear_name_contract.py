"""Verify clear-selection action names overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationClearSelection"',
    'Accessible.name: "Clear visualization selection, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: enabled ? "available" : "unavailable"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing clear selection markers: " + ", ".join(missing))
print(f"Visualization clear name contract verified ({len(required)} checks)")
