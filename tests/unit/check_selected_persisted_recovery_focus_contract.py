"""Verify selected persisted recovery controls preserve focus and accessibility."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "discardPersistedRecovery"',
    'Accessible.name: "Discard selected persisted analysis run"',
    "Focus returns here after confirmation.",
    "discardPersistedRecoveryButton.forceActiveFocus()",
    "onRejected: discardPersistedRecoveryButton.forceActiveFocus()",
    'objectName: "retryPersistedRecovery"',
    'Accessible.name: "Retry selected persisted analysis run"',
    "reset status cleared.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing selected persisted recovery focus markers: " + ", ".join(missing))
print(f"Selected persisted recovery focus contract verified ({len(required)} checks)")
