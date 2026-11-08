"""Verify native monitor callbacks persist the replacement display target."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/platform/windows_overlay.hpp").read_text(encoding="utf-8")
cpp = (root / "src/desktop/platform/windows_overlay.cpp").read_text(encoding="utf-8")
required = [
    "void persistCurrentDisplay();",
    "void OverlayController::persistCurrentDisplay()",
    'settings_->setValue(QStringLiteral("overlay/displayName"), screen->name())',
    "screenRemoved",
    "screenAdded",
    "persistCurrentDisplay(); emit geometryChanged(); emit screensChanged();",
]
missing = [marker for marker in required if marker not in header + cpp]
if missing:
    raise SystemExit("Missing native display persistence markers: " + ", ".join(missing))
print(f"Overlay native display persistence contract verified ({len(required)} checks)")
