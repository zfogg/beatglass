from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function showOverlayAndHideControls()")
end = qml.index("    function unlockOverlayAndRestoreFocus()", start)
block = qml[start:end]

assert "window.restoreOverlayVisualStateFromBridge()" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "DJ overlay visual state unavailable; control window remains available" in block
print("overlay show visual failure fallback contract: ok")
