"""Verify the GUI output/device picker exposes its required states and actions."""
from pathlib import Path


qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "outputPickerDialog"',
    'objectName: "outputPickerSearch"',
    'objectName: "outputPickerState"',
    'objectName: "outputPickerList"',
    'objectName: "refreshOutputPicker"',
    'objectName: "openOutputPicker"',
    'modelData.default ? " · default"',
    'selectedEndpoint === modelData.id',
    'endpointSelectionBridge.error',
    'Keys.onEscapePressed: outputPickerDialog.reject()',
    'endpointSelectionBridge.select(modelData.id)',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("missing output picker markers: " + ", ".join(missing))
print("output picker contract: verified")
