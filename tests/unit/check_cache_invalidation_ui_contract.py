"""Verify cache invalidation is exposed as a worker-owned Library action."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge_h = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
bridge_cpp = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "invalidateCached(QString content",
    'QMetaObject::invokeMethod(worker_, "invalidateCached"',
    "storage().invalidate_cached(identity)",
    'objectName: "invalidateAnalysisCache"',
    "Source files are not changed.",
]
missing = [marker for marker in required if marker not in bridge_h + bridge_cpp + qml]
if missing:
    raise SystemExit("Missing cache invalidation UI markers: " + ", ".join(missing))
print(f"Cache invalidation UI contract verified ({len(required)} checks)")
