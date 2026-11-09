"""Executable contract for queueing bounded recursive analysis."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
assert 'objectName: "analyzeRecursiveResults"' in qml
assert "submitSelectedFiles(paths)" in qml
assert "submitSelectedFiles" in header
assert "submitBatch(std::move(operations))" in source
assert "fingerprint_file" in source
print("Issue #76 recursive batch queue contract verified")
