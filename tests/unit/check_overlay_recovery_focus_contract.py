"""Source contract for missing-display recovery focus and announcement."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QML = (ROOT / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "forgetUnavailableOverlayDisplay"')
    source = QML[start:start + 1100]
    for marker in ("activeFocusOnTab: true", "forgetUnavailableOverlayDisplay()", "Saved overlay display cleared", "overlayDisplay.forceActiveFocus()"):
        assert marker in source, marker
    print("overlay recovery focus contract: verified")

if __name__ == "__main__":
    main()
