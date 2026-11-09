"""Contract checks that cache actions accept only documented cache modes."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    assert "analysisCacheIdentityReady" in QML
    assert '(cacheMode.text === "offline" || cacheMode.text === "live")' in QML
    for object_name in ("lookupAnalysisCache", "invalidateAnalysisCache", "reanalyzeInvalidatedCache"):
        start = QML.index(f'objectName: "{object_name}"')
        block = QML[start:QML.index("\n", start)]
        assert "analysisCacheIdentityReady" in block, object_name

if __name__ == "__main__":
    main()
