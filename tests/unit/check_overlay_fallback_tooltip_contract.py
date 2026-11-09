from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert "Overlay window actions are unavailable" in qml
assert "control-window fallback is active" in qml
assert "overlay settings are retained" in qml
assert 'objectName: "overlayActions"' in qml
assert 'Accessible.role: Accessible.Grouping' in qml
print("Overlay fallback tooltip contract verified (5 checks)")
