"""Verify each theme card exposes palette and keyboard state to assistive tech."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'Accessible.name: window.themeNames[index] + " theme preview"',
    'Accessible.value: (checked ? "selected" : "not selected")',
    'Palette background " + window.themeBackgrounds[index]',
    'accent " + window.themeAccentColors[index]',
    'foreground " + window.themeForegrounds[index]',
    'Use Left and Right to move, Home and End to jump, or Escape',
    'Keys.onLeftPressed:',
    'Keys.onRightPressed:',
    'Keys.onHomePressed:',
    'Keys.onEndPressed:',
    'Keys.onEscapePressed:',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing theme accessibility markers: " + ", ".join(missing))
print(f"Theme accessibility contract verified ({len(required)} checks)")
