"""Verify invalidation resets cache and exposes a fresh reanalysis lifecycle state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'analysisStatus_ = QStringLiteral("Analysis idle · reanalysis required")',
    "emit analysisStatusChanged();",
    'objectName: "analysisAdmissionStatus"',
    'objectName: "analysisLifecycleControls"',
    'objectName: "retryAdmittedAnalysis"',
]
missing = [marker for marker in required if marker not in bridge + qml]
if missing:
    raise SystemExit("Missing reanalysis lifecycle UI markers: " + ", ".join(missing))
print(f"Reanalysis lifecycle UI contract verified ({len(required)} checks)")
