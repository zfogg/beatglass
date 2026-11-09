from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
start = qml.index('objectName: "overlayInteractionMode"')
block = qml[start:start + 800]
assert 'recovery pending; settings retained' in block
assert 'Recovery is pending; overlay settings are retained.' in block
assert 'Accessible.role: Accessible.StaticText' in block
assert 'Input mode' in block
assert 'Current overlay pointer interaction mode' in block
print("Overlay interaction retention contract verified (5 checks)")
