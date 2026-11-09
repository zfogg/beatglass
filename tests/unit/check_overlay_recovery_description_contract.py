from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert "recovery is pending" in qml
assert "control-window fallback remains available" in qml
assert "persisted settings are retained" in qml
assert 'objectName: "overlayActions"' in qml
assert 'Accessible.role: Accessible.Grouping' in qml
print("Overlay recovery description contract verified (5 checks)")
