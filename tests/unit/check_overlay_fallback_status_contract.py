from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert "native overlay launch fallback is active" in qml
assert "control window remains available" in qml
assert "overlay settings are retained" in qml
assert 'Accessible.role: Accessible.Grouping' in qml
assert 'objectName: "overlayActions"' in qml
print("Overlay fallback status contract verified (5 checks)")
