"""Executable contract for confirming cache reanalysis."""
from pathlib import Path

qml = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "reanalyzeInvalidatedCache"')
block = qml[start:start + 2600]
for marker in ("reanalyzeCacheDialog.open()", 'objectName: "reanalyzeCacheDialog"', "storageRecovery.reanalyze(", "source audio file is not changed"):
    assert marker in block or marker in qml, marker
print("Issue #76 reanalysis confirmation contract verified")
