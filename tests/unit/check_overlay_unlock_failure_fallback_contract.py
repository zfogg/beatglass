from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function unlockOverlayAndRestoreFocus()")
end = qml.index("    function reapplyOverlaySettingsAndRestore()", start)
block = qml[start:end]

assert "window.overlayBridge.setClickThrough(false)" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "control-window fallback remains available" in block
print("overlay unlock failure fallback contract: ok")
