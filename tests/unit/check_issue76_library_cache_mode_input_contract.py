"""Contract checks that the cache mode field rejects undocumented modes."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "analysisCacheMode"')
    block = QML[start:QML.index("\n", start)]
    assert "RegularExpressionValidator" in block
    assert "/^(offline|live)$/" in block
    assert 'invalid; use offline or live' in block

if __name__ == "__main__":
    main()
