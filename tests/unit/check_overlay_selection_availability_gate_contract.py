from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function restoreOverlayDisplaySelectionFromBridge()")
end = qml.index("    function restoreOverlayVisualStateFromBridge()", start)
block = qml[start:end]

assert "!window.overlayBridge.available" in block
assert "window.overlayDisplaySelectionRequired = !window.overlayBridge.available" in block
assert "return window.overlayBridge.available && savedIndex >= 0" in block
print("overlay selection availability gate contract: ok")
