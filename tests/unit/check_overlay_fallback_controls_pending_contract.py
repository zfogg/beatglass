from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function restoreOverlayFallbackControls(reason)")
end = qml.index("    function showOverlayAndHideControls()", start)
block = qml[start:end]

assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
assert "window.show()" in block
print("overlay fallback controls pending contract: ok")
