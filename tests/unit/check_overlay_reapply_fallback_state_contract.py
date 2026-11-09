"""Verify reapply failures activate overlay fallback state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
block = qml[qml.index('function reapplyOverlaySettingsAndRestore()'):]
block = block[:block.index('function recoverOverlayAfterDisplayChange()')]
if block.count("window.overlayLaunchFallbackActive = true") < 3:
    raise SystemExit("Missing reapply fallback state handling")
print("Overlay reapply fallback state contract verified (1 check)")
