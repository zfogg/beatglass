"""Verify the selected Library track has an explicit playback reopen path."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    for marker in (
        "function reopenSelectedPlayback()",
        "openSource(window.selectedAudioUrl, nextEpoch)",
        "replaceSource(window.selectedAudioUrl, nextEpoch)",
        'objectName: "reopenSelectedPlayback"',
        "Selected Library track reopened",
    ):
        assert marker in QML, marker
    print("Issue #76 selected playback reopen contract verified (5 checks)")


if __name__ == "__main__":
    main()
