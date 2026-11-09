from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function syncOverlaySettingsStateFromBridge()")
end = qml.index("    function syncOverlaySettingsFromBridge()", start)
block = qml[start:end]

assert "!isFinite(opacity) || opacity < 0.2 || opacity > 1.0 || !geometryReady" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
print("overlay invalid settings fallback contract: ok")
