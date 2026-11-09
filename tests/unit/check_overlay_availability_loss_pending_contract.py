from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("        function onAvailableChanged()")
end = qml.index("    Component.onCompleted:", start)
block = qml[start:end]

hide = block.index("window.hideOverlayAndRestoreControls()")
after_hide = block[hide:]
assert "window.overlayDisplayRecoveryPending = true" in after_hide
assert "window.overlayDisplaySelectionRequired = true" in after_hide
print("overlay availability-loss pending contract: ok")
