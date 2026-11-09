"""Verify forgetting a saved display activates fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlaySettingsRevision += 1\n        window.overlayLaunchFallbackActive = true\n        window.overlayDisplaySelectionRequired = true'
if marker not in qml:
    raise SystemExit("Missing forget-display fallback state")
print("Overlay forget fallback contract verified (1 check)")
