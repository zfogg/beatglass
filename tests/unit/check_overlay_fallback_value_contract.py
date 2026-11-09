from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert 'objectName: "overlayActions"' in qml
assert 'unavailable; control-window fallback active; settings retained' in qml
assert 'Accessible.role: Accessible.Grouping' in qml
assert 'settings retained' in qml
assert 'window.overlayBridge && window.overlayBridge.available' in qml
print("Overlay fallback value contract verified (5 checks)")
