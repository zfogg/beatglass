from pathlib import Path

root = Path(__file__).parents[2]
header = (root / "src/desktop/library_preferences.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/library_preferences.cpp").read_text(encoding="utf-8")
for marker in ("Q_PROPERTY(QVariantList roots", "modelVersion", "addFolder",
                "setFolderScanState", "library/model", "library/folder",
                "sourceId", "lastError", "canonicalFilePath"):
    if marker not in header + source:
        raise SystemExit(f"Missing Library model marker: {marker}")
print("versioned multi-folder Library model contract verified")
