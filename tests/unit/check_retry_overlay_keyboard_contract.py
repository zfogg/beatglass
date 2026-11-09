"""Source contract for keyboard-reachable overlay retry."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "retryOverlay"')
    source = QML[start:start + 1000]
    assert "activeFocusOnTab: true" in source
    assert "Accessible.role: Accessible.Button" in source
    assert "announceCaptureRecoveryState" in source
    print("retry overlay keyboard contract: verified")

if __name__ == "__main__":
    main()
