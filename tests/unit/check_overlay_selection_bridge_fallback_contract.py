from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function restoreOverlayDisplaySelectionFromBridge()")
end = qml.index("    function restoreOverlayVisualStateFromBridge()", start)
block = qml[start:end]

assert "if (!window.overlayBridge)" in block
assert "window.overlayDisplaySelectionRequired = true" in block
assert "window.overlayLaunchFallbackActive = true" in block
print("overlay selection bridge fallback contract: ok")
