from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function syncOverlayGeometryStateFromBridge()")
end = qml.index("    function syncOverlaySettingsStateFromBridge()", start)
block = qml[start:end]

assert "window.overlayBridge.geometry.width <= 0" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
print("overlay invalid geometry fallback contract: ok")
