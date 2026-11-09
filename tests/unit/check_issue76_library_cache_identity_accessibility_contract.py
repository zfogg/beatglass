"""Contract checks for the Library analysis-cache identity form."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    for object_name, accessible_name in (
        ("analysisCacheContent", "Analysis cache content identity"),
        ("analysisCacheEngine", "Analysis cache engine"),
        ("analysisCacheConfig", "Analysis cache configuration"),
        ("analysisCacheMode", "Analysis cache mode"),
    ):
        start = QML.index(f'objectName: "{object_name}"')
        block = QML[start:QML.index("\n", start)]
        assert f'Accessible.name: "{accessible_name}"' in block
        assert "Accessible.description:" in block
    assert QML.index('objectName: "analysisCacheContent"') < QML.index('objectName: "lookupAnalysisCache"')
    assert "cacheContent.text !== \"\"" in QML[QML.index('objectName: "lookupAnalysisCache"'):]

if __name__ == "__main__":
    main()
