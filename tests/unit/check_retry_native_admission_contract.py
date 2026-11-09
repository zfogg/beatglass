"""Source contract for retry-native click-time admission."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "retryNativePlayback"')
    source = QML[start:start + 1000]
    assert "nativeError !== \"\"" in source
    assert "retryNative()" in source
    assert "retry is no longer available" in source
    print("retry native admission contract: verified")

if __name__ == "__main__":
    main()
