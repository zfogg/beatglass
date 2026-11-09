"""Executable contract for safe cache invalidation confirmation."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "invalidateAnalysisCache"')
block = qml[start:start + 4000]
for marker in ("invalidateCacheDialog.open()", 'objectName: "invalidateCacheDialog"', "invalidateCached(cacheContent.text"):
    assert marker in block, marker
assert "source audio file is not changed" in qml
print("Issue #76 cache invalidation confirmation contract verified")
