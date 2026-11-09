"""Verify playback source identity is exposed beside Library playback."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "playbackSourceIdentityStatus"')
    block = QML[start:start + 1000]
    for marker in ("selected Library source", "differs from selected Library source", "Accessible.role: Accessible.Status", "source audio remains unchanged"):
        assert marker in block, marker
    print("Issue #76 playback source identity contract verified (4 checks)")

if __name__ == "__main__":
    main()
