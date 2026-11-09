"""Executable contract for guarding cache invalidation to confirmed hits."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "invalidateAnalysisCache"')
block = qml[start:start + 1800]
assert "storageRecovery.cacheHit === true" in block
assert "Run Check cache and confirm a completed cache hit" in block
print("Issue #76 cache invalidation hit guard contract verified")
