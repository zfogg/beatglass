"""Verify overlay harmony communicates recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'objectName: "overlayHarmony"; color: window.themeForegrounds[window.selectedThemeIndex]; Accessible.name: "Overlay harmony: " + text; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing overlay harmony recovery marker")
print("Overlay harmony recovery contract verified (1 check)")
