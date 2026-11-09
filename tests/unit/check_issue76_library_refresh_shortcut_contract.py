"""Verify Ctrl+R refreshes the focused Library folder."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    assert 'objectName: "libraryRefreshShortcut"' in QML
    assert 'sequence: "Ctrl+R"' in QML
    assert 'files.refresh(); window.refreshCatalog()' in QML
    print("Issue #76 Library refresh shortcut contract verified")


if __name__ == "__main__":
    main()
