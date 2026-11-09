from pathlib import Path

qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
assert 'objectName: "overlayActions"' in qml
assert 'settings retained; revision' in qml
assert 'window.overlaySettingsRevision' in qml
assert 'Accessible.role: Accessible.Grouping' in qml
assert 'window.overlayHiddenByUser' in qml
print("Overlay available value contract verified (5 checks)")
