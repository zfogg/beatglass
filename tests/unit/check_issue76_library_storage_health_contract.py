"""Verify persisted storage health is visible from the Library surface."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "libraryStorageRecoveryHealth"',
    'storageRecovery.status',
    'storageRecovery.retryableRunIds.length',
    'storageRecovery.status === "Storage closed"',
    'Reopen storage before retrying or discarding saved analysis runs.',
    'objectName: "reopenStorage"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise AssertionError("missing Library storage health markers: " + ", ".join(missing))
print(f"Library storage health contract: {len(required)} checks passed")
