"""Executable contract for click-through settings revision updates."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    marker = 'function onClickThroughChanged()'
    start = QML.index(marker)
    block = QML[start:QML.index('        }', start)]
    assert 'window.overlaySettingsRevision += 1' in block
    assert 'DJ overlay is click-through' in block
    assert 'DJ overlay interaction restored' in block
    print("Overlay click-through revision contract verified (3 checks)")


if __name__ == "__main__":
    main()
