from pathlib import Path

ROOT = Path(__file__).parents[2]
QML = (ROOT / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
PREFS = (ROOT / "src" / "desktop" / "library_preferences.hpp").read_text(encoding="utf-8")
CPP = (ROOT / "src" / "desktop" / "library_preferences.cpp").read_text(encoding="utf-8")

required = [
    'Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)',
    'Q_INVOKABLE void addFolder(const QString& folder)',
    'Q_INVOKABLE void removeFolder(int index)',
    'settings_.value(QStringLiteral("library/folders"))',
    'objectName: "libraryDirectoriesSettings"',
    'objectName: "addLibraryDirectory"',
    'objectName: "removeLibraryDirectory"',
    'libraryPreferences.addFolder(selectedFolder.toLocalFile())',
    'libraryPreferences.removeFolder(index)',
    'text: "Add directory…"',
]
missing = [marker for marker in required if marker not in (QML + PREFS + CPP)]
if missing:
    raise SystemExit("missing library directory list GUI markers: " + ", ".join(missing))
print("library directory list GUI contract passed")
