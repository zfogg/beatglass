"""Executable UI contract for issue #4788's Add Folder workflow."""
from pathlib import Path

qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")
required = (
    'objectName: "addFolder"', 'objectName: "addFolderDialog"', 'objectName: "addFolderPath"',
    'objectName: "addFolderProgress"', 'objectName: "addFolderRetry"', 'objectName: "addFolderCancel"',
    'window.addFolderState === "duplicate"', 'window.addFolderState === "scanning"',
    'window.addFolderState === "error"', 'window.addFolderState === "success"',
    'addFolderScanTimer.stop()', 'window.libraryCatalogBridge.browseRecursive(pendingAddFolder',
    'libraryPreferences.folder = pendingAddFolder.toLocalFile()',
)
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit(f"missing issue #4788 Add Folder markers: {missing}")
print("issue #4788 Add Folder popup contract passed")
