from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("        function onAvailableChanged()")
end = qml.index("    Component.onCompleted:", start)
block = qml[start:end]

assert "if (!window.overlayBridge.available)" in block
assert "window.overlayLaunchFallbackActive = true" in block
assert "window.overlayDisplayRecoveryPending = true" in block
print("overlay availability-loss fallback contract: ok")
