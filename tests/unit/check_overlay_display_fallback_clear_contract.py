"""Verify selecting a display clears fallback-active state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlaySettingsRevision += 1\n        window.overlayLaunchFallbackActive = false\n        window.overlayDisplaySelectionRequired = false'
if marker not in qml:
    raise SystemExit("Missing display fallback clear")
print("Overlay display fallback clear contract verified (1 check)")
