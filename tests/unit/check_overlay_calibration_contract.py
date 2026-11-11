"""Verify the GUI overlay calibration transaction remains wired in QML."""
from pathlib import Path


qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayCalibrationButton"',
    'objectName: "overlayCalibrationDialog"',
    'objectName: "overlayCalibrationPreview"',
    'objectName: "overlayCalibrationCancel"',
    'objectName: "overlayCalibrationSave"',
    'function rollbackOverlayCalibration()',
    'function saveOverlayCalibration()',
    'sequence: "Ctrl+Alt+Shift+Enter"',
    'moveToScreen(currentIndex)',
    'setOpacity(value)',
    'setClickThrough(checked)',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("missing overlay calibration markers: " + ", ".join(missing))
print("overlay calibration contract: verified")
