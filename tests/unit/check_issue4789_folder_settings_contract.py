"""Validate the GUI folder-management Settings contract."""
from pathlib import Path

qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
for marker in (
    'objectName: "libraryDirectoriesSettings"',
    'model: typeof libraryPreferences !== "undefined" ? libraryPreferences.roots : []',
    'objectName: "addLibraryDirectory"',
    'objectName: "removeLibraryDirectory"',
    'objectName: "moveLibraryDirectoryUp"',
    'objectName: "moveLibraryDirectoryDown"',
    'objectName: "rescanLibraryDirectory"',
    'objectName: "revealLibraryDirectory"',
    'objectName: "libraryDirectoryStatus"',
    'libraryPreferences.moveFolder(sourceId, -1)',
    'libraryPreferences.moveFolder(sourceId, 1)',
    'libraryPreferences.setFolderScanState(sourceId, "scanning", "")',
    'window.refreshCatalog()',
    'root.sourceId)'
):
    if marker not in qml:
        raise SystemExit(f"missing folder Settings marker: {marker}")

header = (Path(__file__).parents[2] / "src" / "desktop" / "library_preferences.hpp").read_text(encoding="utf-8")
source = (Path(__file__).parents[2] / "src" / "desktop" / "library_preferences.cpp").read_text(encoding="utf-8")
for marker in ('moveFolder(const QString& sourceId, int delta)', 'roots_.move(from, to)'):
    if marker not in header + source:
        raise SystemExit(f"missing folder model marker: {marker}")
print("issue #4789 folder Settings contract: PASS")
