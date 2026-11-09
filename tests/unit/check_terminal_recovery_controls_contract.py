"""Verify terminal retry/discard controls keep confirmation, focus, and accessibility."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "discardPersistedRecovery"',
    "discardRecoveryDialog.open()",
    "This removes only the retry record.",
    "discardPersistedRecoveryButton.forceActiveFocus()",
    'objectName: "retryPersistedRecovery"',
    "activeFocusOnTab: true",
    "storageRecovery.retrySelectedRun(); forceActiveFocus()",
    "Accessible.name: \"Retry selected persisted analysis run\"",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing terminal recovery control markers: " + ", ".join(missing))
print(f"Terminal recovery controls contract verified ({len(required)} checks)")
