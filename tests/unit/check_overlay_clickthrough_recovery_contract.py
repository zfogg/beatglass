"""Executable contract for click-through controls during bridge recovery."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")


def main() -> None:
    checks = [
        'clickThroughToggle.checked = false',
        'clickThroughToggle.checked = window.overlayBridge.clickThrough',
        'settings revision " + window.overlaySettingsRevision',
        'window.overlayBridge.available && window.overlayBridge.clickThrough',
        'Overlay is not in click-through mode or is unavailable',
    ]
    for fragment in checks:
        assert fragment in QML, f"missing contract fragment: {fragment}"
    print(f"Overlay click-through recovery contract verified ({len(checks)} checks)")


if __name__ == "__main__":
    main()
