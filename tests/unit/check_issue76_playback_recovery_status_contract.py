"""Verify Library playback exposes actionable recovery/source-alignment state."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "playbackRecoveryStatus"')
    block = QML[start:start + 1100]
    for marker in ("nativeError", "reopen the selected source", "Accessible.role: Accessible.Status", "selected audio source remains unchanged"):
        assert marker in block, marker
    print("Issue #76 playback recovery status contract verified (4 checks)")

if __name__ == "__main__":
    main()
