"""Validate the GUI popup interaction contracts for issue #4780."""
from pathlib import Path


qml = (Path(__file__).parents[2] / "src" / "desktop" / "qml" / "Main.qml").read_text(encoding="utf-8")


def require(text: str, description: str) -> None:
    if text not in qml:
        raise SystemExit(f"missing popup contract: {description}")


for object_name in (
    "addFolderDialog",
    "addFolderConfirm",
    "addFolderRetry",
    "addFolderCancel",
    "removeFolderDialog",
    "removeFolderConfirm",
    "removeFolderCancel",
    "capturePermissionDialog",
    "capturePermissionRetry",
    "capturePermissionCancel",
    "outputPickerDialog",
    "overlayCalibrationDialog",
):
    require(f'objectName: "{object_name}"', object_name)

require("onOpened: addFolderConfirm.forceActiveFocus()", "add-folder initial focus")
require("onRejected: addFolder.forceActiveFocus()", "folder chooser focus return")
require("onRejected: { removeFolderDialog.close(); addLibraryDirectory.forceActiveFocus() }", "remove-folder Escape focus return")
require("onRejected: { capturePermissionDialog.close(); openCapturePermissionRecovery.forceActiveFocus() }", "permission Escape focus return")
require("Source audio and files are not changed", "remove-folder safety copy")
require("capturePermissionDialog.open()", "permission recovery entry point")
require("window.endpointSelectionBridge.refresh()", "permission recovery retry")

print("popup interaction flows: add/remove/permission/calibration/output contracts present")
