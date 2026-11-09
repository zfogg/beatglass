from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayFallbackReviewAction"')
end = qml.index('objectName: "overlayGeometryStatus"', start)
block = qml[start:end]

assert "window.overlayLaunchFallbackActive || window.overlayDisplayRecoveryPending" in block
assert "Review recovery" in block
assert "overlaySettingsSummary.forceActiveFocus()" in block
assert "Overlay recovery workflow focused" in block
print("overlay fallback primary action UI contract: ok")
