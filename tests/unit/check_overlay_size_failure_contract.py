"""Verify failed native overlay resize keeps the persisted size selection."""

from pathlib import Path


QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    start = QML.index('objectName: "overlaySize"')
    block = QML[start:start + 1800]
    assert "const resized = window.overlayBridge.resizeOverlay" in block
    assert "if (resized)" in block
    assert "syncOverlaySettingsStateFromBridge" in block
    assert "persisted size remains active" in block
    print("Overlay size failure contract verified (4 checks)")


if __name__ == "__main__":
    main()
