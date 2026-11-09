from pathlib import Path

readme = Path("README.md").read_text(encoding="utf-8")
qml = Path("src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for theme in ("Light", "Dark", "High contrast", "Purple", "Blue", "Yellow"):
    assert theme in readme
    assert theme in qml
assert "reduced-motion preference" in readme
assert 'property var themeNames: ["Light", "Dark", "High contrast", "Purple", "Blue", "Yellow"]' in qml
print("README/theme status contract verified (8 checks)")