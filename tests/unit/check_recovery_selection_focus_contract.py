"""Verify persisted recovery selection is announced and remains keyboard-focused."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoveryRuns"',
    "id: persistedRecoveryRuns",
    "storageRecovery.selectRun(currentText)",
    "Selected persisted recovery run ",
    "Retry and discard controls are now available",
    "persistedRecoveryRuns.forceActiveFocus()",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing recovery selection focus markers: " + ", ".join(missing))
print(f"Recovery selection focus contract verified ({len(required)} checks)")
