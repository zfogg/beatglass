"""Verify selected persisted recovery exposes reset status and announcements."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "selectedPersistedRecoveryResetStatus"',
    'storageRecovery.resetStatus',
    'window.announceVisualizationRecovery("Persisted recovery retry admitted; reset status cleared.")',
    'window.announceVisualizationRecovery("Persisted recovery discarded; selected retry state cleared.")',
    'objectName: "retryPersistedRecovery"',
    'objectName: "discardPersistedRecovery"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing selected persisted recovery status markers: " + ", ".join(missing))
print(f"Selected persisted recovery status contract verified ({len(required)} checks)")
