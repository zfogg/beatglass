"""Verify the Library selected-track playback action is wired and accessible."""
from pathlib import Path


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
    start = qml.index('objectName: "playSelectedTrack"')
    block = qml[start : start + 900]
    checks = (
        'objectName: "playSelectedTrack"',
        'Accessible.name: "Play selected Library track"',
        "window.playSelectedLibraryTrack()",
        "window.selectedAudioUrl",
        "playSelectedTrack.forceActiveFocus()",
    )
    for marker in checks:
        assert marker in block, marker
    print(f"Selected playback action contract verified ({len(checks)} checks)")


if __name__ == "__main__":
    main()
