"""Executable contract for native click-through failure feedback."""

from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    for fragment in ("const requestedClickThrough = checked", "window.overlayBridge.setClickThrough(requestedClickThrough)", "if (checked !== requestedClickThrough)", "could not change input mode"):
        assert fragment in QML, f"missing contract fragment: {fragment}"
    print("Overlay click-through failure contract verified (4 checks)")


if __name__ == "__main__":
    main()
