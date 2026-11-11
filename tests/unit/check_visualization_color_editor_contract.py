"""Verify GUI visualization color controls remain connected to persisted preferences."""
from pathlib import Path


qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "visualizationColorsLabel"',
    'objectName: "visualizationColorControls"',
    'objectName: "lowVisualizationColor"',
    'objectName: "midVisualizationColor"',
    'objectName: "highVisualizationColor"',
    'lowColorDialog.open()',
    'midColorDialog.open()',
    'highColorDialog.open()',
    'visualizationBandColor(ratio)',
    'function resetAppearancePreferences()',
    'window.themePreferencesBridge.reset()',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("missing visualization color markers: " + ", ".join(missing))
print("visualization color editor contract: verified")
