"""Verify persisted analysis queue progress is surfaced through bridge and QML."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "Q_PROPERTY(qulonglong analysisFrames",
    "void analysisProgress(qulonglong frames);",
    "handle_->frames()",
    "analysisFrames_ = frames; emit analysisProgressChanged();",
    'objectName: "persistedAnalysisProgress"',
    '"Persisted analysis progress: " + storageRecovery.analysisFrames',
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing persisted queue progress markers: " + ", ".join(missing))
print(f"Persisted queue progress contract verified ({len(required)} checks)")
