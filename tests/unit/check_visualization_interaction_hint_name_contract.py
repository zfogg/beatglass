"""Verify interaction hints name overlay recovery context."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationInteractionHint"',
    'Accessible.name: "Visualization interaction hint, " + (window.overlayDisplayRecoveryPending ? "overlay recovery pending" : "overlay ready")',
    'Accessible.value: text',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing interaction hint markers: " + ", ".join(missing))
print(f"Visualization interaction hint contract verified ({len(required)} checks)")
