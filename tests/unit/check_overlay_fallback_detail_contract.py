from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert qml.count("overlay settings are retained") >= 3
assert "DJ overlay unavailable; control window remains available" in qml
assert "DJ overlay visual state unavailable; control window remains available" in qml
assert "DJ overlay could not be restored; control window remains available" in qml
assert "tray/taskbar fallback" in qml
print("Overlay fallback detail contract verified (5 checks)")
