from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'Q_PROPERTY(QString recoveryStatus READ recoveryStatus NOTIFY statusChanged)',
    'recoveryStatus_ = QStringLiteral("Single analysis recovery discarded")',
    'recoveryStatus_ = QStringLiteral("Batch analysis recovery discarded")',
    'objectName: "analysisRecoveryStatus"',
    'Accessible.role: Accessible.Status',
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing recovery reset status markers: " + ", ".join(missing))
print(f"Recovery reset status contract verified ({len(required)} checks)")
