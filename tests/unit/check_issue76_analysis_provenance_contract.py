"""Executable contract for analysis provenance presentation."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
assert 'objectName: "analysisProvenance"' in qml
for marker in ("engineVersion", "configurationId", "analysisMode", "provenanceChanged"):
    assert marker in header
assert "emit provenanceChanged()" in source
print("Issue #76 analysis provenance contract verified")
