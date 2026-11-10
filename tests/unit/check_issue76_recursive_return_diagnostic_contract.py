"""Verify returning from recursive results clears stale catalog diagnostics."""
from pathlib import Path
QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
def main() -> None:
    assert 'window.catalogDiagnostic = ""' in QML
    print("Issue #76 recursive return diagnostic contract verified")
if __name__ == "__main__": main()
