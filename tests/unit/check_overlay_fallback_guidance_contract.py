"""Source contract for actionable overlay fallback guidance."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayFallbackState"')
    source = QML[start:start + 500]
    assert "Retry overlay" in source
    assert "Accessible.value: text" in source
    assert "Accessible.role: Accessible.Status" in source
    print("overlay fallback guidance contract: verified")

if __name__ == "__main__":
    main()
