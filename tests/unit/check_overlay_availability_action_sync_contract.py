"""Executable contract for overlay availability/action recovery synchronization."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def require(fragment: str) -> None:
    assert fragment in QML, f"missing contract fragment: {fragment}"


def main() -> None:
    require('function onAvailableChanged()')
    require('window.overlayDisplayRecoveryPending = true')
    require('window.syncOverlaySettingsStateFromBridge()')
    require('window.overlayBridge.screenNames.length === 0')
    require('window.overlayDisplaySelectionRequired = true')
    require('DJ overlay recovered; choose a display to re-enable overlay actions.')
    require('window.overlayDisplayRecoveryPending = false')
    require('DJ overlay recovered; Show, Restore, and Reapply actions are available.')
    print("Overlay availability action sync contract verified (8 checks)")


if __name__ == "__main__":
    main()
