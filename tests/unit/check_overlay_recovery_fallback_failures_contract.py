from pathlib import Path


qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index("    function recoverOverlayAfterDisplayChange()")
end = qml.index("    function forgetUnavailableOverlayDisplay()", start)
block = qml[start:end]

assert block.count("window.overlayLaunchFallbackActive = true") == 4
print("overlay recovery fallback failure contract: ok")
