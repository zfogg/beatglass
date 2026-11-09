"""Verify analysisError cannot survive a new queue/recovery transition."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
submit = source[source.index("void submitAnalysis("):source.index("void cancelAnalysis(")]
assert "Analysis submission rejected:" in submit
assert "handle_.reset(); emit result(false" in submit
retry = source[source.index("void retryAnalysis()"):source.index("void admitAnalysis(")]
assert "Analysis retry rejected:" in retry
assert "handle_.reset(); emit result(false" in retry
assert "Analysis retry rejected" in retry
cancel = source[source.index("void cancelAnalysis()"):source.index("void retryAnalysis()")]
assert "Analysis cancellation rejected:" in cancel
assert "catch (const std::exception& error)" in cancel
admit = source[source.index("void admitAnalysis("):source.index("signals:")]
assert "Analysis admission rejected:" in admit
assert "emit admitted(false, QString{})" in admit
assert "Analysis admission rejected" in admit
analysis_connection = source[source.index("connect(worker_, &Worker::analysis,"):source.index("connect(worker_, &Worker::analysisError,")]
assert "Analysis cancelled" in analysis_connection and "analysisResult_.clear()" in analysis_connection
docs = (root / "docs/progress.md").read_text(encoding="utf-8")
required = [
    'text != QStringLiteral("Analysis failed")',
    "analysisError_.clear(); emit analysisErrorChanged();",
    "queued, running, cancelled, completed",
    "restart-recovery transitions clear the previous error",
]
missing = [marker for marker in required if marker not in source + docs]
if missing:
    raise SystemExit("Missing analysis error lifecycle markers: " + ", ".join(missing))
print(f"Analysis error lifecycle contract verified ({len(required)} checks)")
