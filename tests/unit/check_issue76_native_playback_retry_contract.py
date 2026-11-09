"""Verify native playback retry reports a user-visible recovery result."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    for marker in (
        "function retryNativePlaybackOutput()",
        "const retried = window.playbackBridge.retryNative()",
        "Native playback output recovered",
        "Native playback output retry failed",
        "onClicked: window.retryNativePlaybackOutput()",
    ):
        assert marker in QML, marker
    print("Issue #76 native playback retry contract verified (5 checks)")


if __name__ == "__main__":
    main()
