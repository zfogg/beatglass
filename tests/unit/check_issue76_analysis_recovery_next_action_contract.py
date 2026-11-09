"""Executable contract for persisted analysis recovery guidance."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "analysisRecoveryNextAction"')
block = qml[start:start + 1400]
for marker in ("Cancelled", "Failed", "Retry batch", "Discard batch", "without changing source audio"):
    assert marker in block, marker
print("Issue #76 analysis recovery next-action contract verified")
