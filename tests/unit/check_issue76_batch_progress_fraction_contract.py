from pathlib import Path

root = Path(__file__).parents[2]
header = (root / "src/desktop/analysis_job_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/analysis_job_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in (
    'Q_PROPERTY(qulonglong batchTotalFrames READ batchTotalFrames NOTIFY batchChanged)',
    'Q_PROPERTY(bool batchProgressKnown READ batchProgressKnown NOTIFY batchChanged)',
    'Q_PROPERTY(double batchProgressFraction READ batchProgressFraction NOTIFY batchChanged)',
    'batch_total_frames_ = source_total',
    'addBatchTotal(qulonglong& total, qulonglong value)',
    'std::numeric_limits<qulonglong>::max() - total',
):
    if marker not in header + source:
        raise SystemExit(f"Missing batch progress marker: {marker}")
if "batchProgressFraction" not in qml or "batchProgressKnown" not in qml:
    raise SystemExit("Batch progress bar does not consume bounded source progress")
print("batch progress fraction contract verified")
