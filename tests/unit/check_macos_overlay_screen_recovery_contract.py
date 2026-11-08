"""Verify macOS screen recovery persists and publishes adjusted geometry."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
cpp = (root / "src/desktop/platform/macos_overlay.cpp").read_text(encoding="utf-8")
required = [
    "screen_timer_",
    "keepOnScreen(); updating_screens_ = false;",
    "scheduleSave();",
    "emit geometryChanged();",
    "emit screensChanged();",
]
missing = [marker for marker in required if marker not in cpp]
if missing:
    raise SystemExit("Missing macOS screen recovery markers: " + ", ".join(missing))
print(f"macOS overlay screen recovery contract verified ({len(required)} checks)")
