"""Verify Backspace navigates the Library browser to its parent folder."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert "Keys.onBackspacePressed:" in QML
    assert 'window.browseFolder = files.parentFolder' in QML
    assert 'browser.currentIndex = -1' in QML
    print("Issue #76 browser parent shortcut contract verified")


if __name__ == "__main__":
    main()
