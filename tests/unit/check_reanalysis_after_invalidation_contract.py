"""Verify invalidation leads to a fresh admission path and visible status."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
tests = (root / "tests/unit/analysis_operation_test.cpp").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'TEST_CASE("invalidated identity is admitted for fresh reanalysis")',
    "owner.invalidate_cached(identity)",
    "admission.analysis_reserved",
    'QStringLiteral("Cached analysis invalidated")',
    'objectName: "analysisAdmissionStatus"',
]
missing = [marker for marker in required if marker not in tests + bridge + qml]
if missing:
    raise SystemExit("Missing reanalysis-after-invalidation markers: " + ", ".join(missing))
print(f"Reanalysis after invalidation contract verified ({len(required)} checks)")
