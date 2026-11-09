from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function restoreOverlayVisualStateFromBridge()")
end = qml.index("    function syncOverlaySizePreferenceFromGeometry", start)
block = qml[start:end]

assert "if (!window.overlayBridge)" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
print("overlay visual bridge fallback contract: ok")
