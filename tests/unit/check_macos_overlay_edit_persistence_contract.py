"""Verify direct macOS display and size edits schedule native persistence."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
cpp = (root / "src/desktop/platform/macos_overlay.cpp").read_text(encoding="utf-8")
required = [
    "bool OverlayController::moveToScreen(int index)",
    "keepOnScreen(); scheduleSave(); return true;",
    "bool OverlayController::resizeOverlay(int width, int height)",
    "overlay_->resize(width, height); keepOnScreen(); scheduleSave(); return true;",
]
missing = [marker for marker in required if marker not in cpp]
if missing:
    raise SystemExit("Missing macOS edit persistence markers: " + ", ".join(missing))
print(f"macOS overlay edit persistence contract verified ({len(required)} checks)")
