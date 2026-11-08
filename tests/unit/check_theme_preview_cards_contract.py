"""Verify theme preview cards expose synchronized palette evidence."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "themePreviews"',
    'objectName: "themePreview" + index',
    'checked: window.selectedThemeIndex === index',
    'objectName: "themePreviewAccent" + index',
    'objectName: "themePreviewForeground" + index',
    'color: window.themeAccentColors[index]',
    'color: window.themeForegrounds[index]',
    'window.themePreferencesBridge.themeIndex = index',
    'function onThemeIndexChanged()',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing theme preview synchronization markers: " + ", ".join(missing))
if qml.count('objectName: "themePreviewAccent" + index') != 1:
    raise SystemExit("each theme card must have one accent preview")
if qml.count('objectName: "themePreviewForeground" + index') != 1:
    raise SystemExit("each theme card must have one foreground preview")
print(f"Theme preview card contract verified ({len(required)} checks)")
