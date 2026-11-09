"""Verify retained-settings status names expose recovery state and revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsRetentionStatus"',
    'Accessible.name: "Overlay settings retention, " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready") + ", revision " + window.overlaySettingsRevision',
    'Accessible.value: text + "; settings revision " + window.overlaySettingsRevision',
    '"Overlay settings retained while recovery is pending"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay retention name markers: " + ", ".join(missing))
print(f"Overlay retention state name contract verified ({len(required)} checks)")
