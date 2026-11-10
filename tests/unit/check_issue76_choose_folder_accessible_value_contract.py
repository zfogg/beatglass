"""Verify the normal Library folder chooser exposes current root state."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'Accessible.value: window.browseFolder.toString() || "No folder selected"' in QML
    print("Issue #76 choose-folder accessible value contract verified")
if __name__ == "__main__": main()
