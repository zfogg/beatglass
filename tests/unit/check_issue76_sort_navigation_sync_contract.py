"""Verify sorting selects the Library tab."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'function toggleLibrarySort(target) {\n        navigation.currentIndex = 0' in QML
    print("Issue #76 sort navigation sync contract verified")
if __name__ == "__main__": main()
