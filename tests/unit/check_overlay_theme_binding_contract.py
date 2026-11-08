"""Verify the overlay responds to persisted theme state and remains legible."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "color: window.themeBackgrounds[window.selectedThemeIndex]",
    "border.color: window.themeAccentColors[window.selectedThemeIndex]",
    "color: window.themeForegrounds[window.selectedThemeIndex]",
    "function onThemeIndexChanged()",
    "window.themePreferenceAnnouncement = \"Theme preference restored: \"",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay theme binding markers: " + ", ".join(missing))
if qml.count("color: window.themeForegrounds[window.selectedThemeIndex]") < 4:
    raise SystemExit("overlay status content must use the active theme foreground")
print(f"Overlay theme binding contract verified ({len(required)} checks)")
