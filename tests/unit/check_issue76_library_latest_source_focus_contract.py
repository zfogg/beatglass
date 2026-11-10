"""Verify the latest published source action has a keyboard focus path."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert 'objectName: "libraryLatestPublishedSourceFocusShortcut"' in QML
    assert 'onActivated: window.focusLibraryLatestPublishedSource()' in QML
    assert 'function focusLibraryLatestPublishedSource()' in QML
    assert 'selectLatestPublishedSource.forceActiveFocus()' in QML
    print("Issue #76 latest published source focus contract verified")


if __name__ == "__main__":
    main()
