"""Verify overlay tempo communicates recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'objectName: "overlayTempo"; color: window.themeForegrounds[window.selectedThemeIndex]; font.pixelSize: 24; font.bold: true; Accessible.name: "Overlay tempo: " + text; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing overlay tempo recovery marker")
print("Overlay tempo recovery contract verified (1 check)")
