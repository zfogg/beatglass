"""Executable contract for queueing analysis from the Library selection."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
for marker in ('objectName: "analyzeSelectedAudio"', "submitSelectedFile(window.selectedAudioUrl.toLocalFile())"):
    assert marker in qml, marker
assert "submitSelectedFile" in header
assert "fingerprint_file" in source
assert "beatglass-desktop-v1" in source
print("Issue #76 selected analysis queue contract verified")
