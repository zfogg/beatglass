"""Executable contract for source-attributed batch result presentation."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
assert 'objectName: "analysisBatchLastSource"' in qml
assert "batchLastSource" in header
assert "batchLastSource_ = sourcePath" in source
assert "sourcePath = QString::fromStdString(operation.source.path.string())" in source
print("Issue #76 batch result source contract verified")
