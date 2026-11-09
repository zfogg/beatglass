"""Verify monitoring guidance communicates overlay recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'objectName: "monitoringGuidance"; wrapMode: Text.WordWrap; Layout.fillWidth: true; Accessible.name: "Monitoring guidance"; Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; overlay recovery pending" : "; overlay ready")'
if marker not in qml:
    raise SystemExit("Missing monitoring guidance recovery marker")
print("Monitoring guidance recovery contract verified (1 check)")
