"""Regression contract consolidating the current-main value from retry PRs #1591/#1522."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8") + (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = ["retryOverlay", "overlayDisplaySelectionRequired", "select a display first", "retrySelectedRun", 'objectName: "overlayUnavailable"', "recovery controls"]
missing = [m for m in required if m not in qml + bridge]
if missing: raise SystemExit("Missing consolidated retry markers: " + ", ".join(missing))
print(f"Consolidated retry successor contract verified ({len(required)} checks)")
