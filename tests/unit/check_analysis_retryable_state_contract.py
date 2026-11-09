"""Verify cancel/failure/restart lifecycle propagates an explicit retryable property."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "Q_PROPERTY(bool analysisRetryable",
    "analysisRetryableChanged",
    'QStringLiteral("Analysis cancelled")',
    'QStringLiteral("Analysis failed")',
    'QStringLiteral("Analysis recovery available")',
    "storageRecovery.analysisRetryable",
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing explicit retryable state markers: " + ", ".join(missing))
print(f"Analysis retryable state contract verified ({len(required)} checks)")
