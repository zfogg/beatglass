from pathlib import Path

root = Path(__file__).parents[2]
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in (
    'Q_PROPERTY(qulonglong totalFrames READ totalFrames NOTIFY progressChanged)',
    'Q_PROPERTY(bool progressKnown READ progressKnown NOTIFY progressChanged)',
    'Q_PROPERTY(double progressFraction READ progressFraction NOTIFY progressChanged)',
    'double progressFraction() const noexcept',
):
    if marker not in header:
        raise SystemExit(f"Missing single-job progress marker: {marker}")
if "progressFraction * 100" not in qml or "totalFrames" not in qml:
    raise SystemExit("Library UI does not expose bounded single-job progress")
print("single-job progress semantics contract verified")
