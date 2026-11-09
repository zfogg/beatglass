from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function hideOverlayAndRestoreControls()")
end = qml.index("    function quitApplication()", start)
block = qml[start:end]

assert "const hidden = !window.overlayBridge || window.overlayBridge.hideOverlay()" in block
assert "if (!hidden)" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "control-window fallback remains active." in block
print("overlay hide failure fallback contract: ok")
