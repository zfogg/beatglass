"""Verify Ctrl+L focuses and selects the Library filter."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert 'objectName: "libraryFilterFocusShortcut"' in QML
    assert 'sequence: "Ctrl+L"' in QML
    assert 'search.forceActiveFocus(); search.selectAll()' in QML
    print("Issue #76 Library filter focus contract verified")


if __name__ == "__main__":
    main()
