"""Verify batch retry exposes admission feedback to the Library UI."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (ROOT / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert "Q_INVOKABLE bool retryBatch();" in header
assert "bool AnalysisJobBridge::retryBatch()" in source
assert "return accepted;" in source
assert "const accepted = window.analysisJobBridge.retryBatch()" in qml
assert "retry was rejected" in qml
print("batch retry admission contract verified")
