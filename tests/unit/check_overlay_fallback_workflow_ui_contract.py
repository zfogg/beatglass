from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayFallbackState"')
end = qml.index('objectName: "overlayGeometryStatus"', start)
block = qml[start:end]

assert "window.overlayLaunchFallbackActive || !window.overlayBridge.available" in block
assert "Review persisted settings, then use Restore overlay." in block
assert "Use Retry overlay after reconnecting a display." in block
print("overlay fallback workflow UI contract: ok")
