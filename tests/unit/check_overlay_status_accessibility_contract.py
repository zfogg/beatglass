"""Source contract for native overlay availability status accessibility."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "overlayUnavailable"')
    source = QML[start:QML.find("\n", start) + 1]
    for marker in ("Accessible.value: text", "Accessible.role: Accessible.Status"):
        assert marker in source, f"missing {marker!r}"
    assert "!window.overlayBridge.available" in QML[start - 100:start]
    print("overlay status accessibility contract: verified")

if __name__ == "__main__":
    main()
