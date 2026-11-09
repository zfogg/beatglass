"""Verify selected-source changes cannot present stale analysis results."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
qml = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "property bool analysisResultMatchesSelection",
    'analysisJobBridge.sourcePath === selectedAudioUrl.toLocalFile()',
    "prior analysis presentation is stale",
    'visible: window.analysisResultMatchesSelection && window.analysisJobBridge.result !== ""',
    'visible: window.analysisResultMatchesSelection && typeof storageRecovery !== "undefined" && storageRecovery.analysisResult !== ""',
    "Prior analysis result hidden because the selected source changed",
    'objectName: "retryAnalysis"',
    'retryAnalysis.forceActiveFocus()',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise AssertionError("missing stale-result contract markers: " + ", ".join(missing))
print(f"Library stale-result contract: {len(required)} checks passed")
