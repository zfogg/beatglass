from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function hideOverlayAndRestoreControls()")
end = qml.index("    function quitApplication()", start)
block = qml[start:end]

failure = block.index("if (!hidden)")
after_failure = block[failure:]
assert "window.overlayLaunchFallbackActive = true" in after_failure
assert "window.overlayDisplayRecoveryPending = true" in after_failure
print("overlay hide failure pending contract: ok")
