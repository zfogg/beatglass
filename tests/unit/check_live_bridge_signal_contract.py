"""Verify the QML live bridge listens to its actual NOTIFY signal."""
from pathlib import Path

qml = Path(__file__).parents[1].joinpath("..", "src", "desktop", "qml", "Main.qml").resolve().read_text(encoding="utf-8")
if "target: window.liveBridge\n        function onChanged()" not in qml:
    raise SystemExit("Main.qml must consume LivePresentation.changed")
fake = Path(__file__).parents[1].joinpath("qml", "tst_navigation.qml").read_text(encoding="utf-8")
for marker in ("signal changed()", "onStatusChanged: changed()", "onTempoChanged: changed()"):
    if marker not in fake:
        raise SystemExit(f"live bridge fake is missing: {marker}")
print("live bridge signal contract verified")
