from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function syncOverlaySizePreferenceFromGeometry(geometry)")
end = qml.index("    function syncOverlayGeometryStateFromBridge()", start)
block = qml[start:end]

assert "geometry.width <= 0 || geometry.height <= 0" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
print("overlay size invalid fallback contract: ok")
