"""Verify playback device selection preserves native device identifiers."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    for marker in ("function playbackDeviceIdAt(index, label)", "label.indexOf(\":\")", "Number(label.slice(0, separator))", "window.playbackDeviceIdAt(currentIndex, currentText)"):
        assert marker in QML, marker
    print("Issue #76 playback device identifier contract verified (4 checks)")


if __name__ == "__main__":
    main()
