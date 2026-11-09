from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function restoreOverlayVisualStateFromBridge()")
end = qml.index("    function syncOverlaySizePreferenceFromGeometry", start)
block = qml[start:end]

assert block.count("window.overlayLaunchFallbackActive = true") == 5
assert block.count("window.overlayDisplayRecoveryPending = true") == 5
print("overlay visual-state failure fallback contract: ok")
