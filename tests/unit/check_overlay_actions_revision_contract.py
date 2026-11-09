"""Verify action-group accessibility reports the same settings revision as summary."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayActions"',
    'settings revision " + window.overlaySettingsRevision',
    'objectName: "overlaySettingsSummary"',
    'window.overlayDisplayRecoveryPending',
    'window.overlayHiddenByUser',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay action revision markers: " + ", ".join(missing))
if qml.count('settings revision " + window.overlaySettingsRevision') < 2:
    raise SystemExit("actions and summary must report the same settings revision")
print(f"Overlay action revision contract verified ({len(required)} checks)")
