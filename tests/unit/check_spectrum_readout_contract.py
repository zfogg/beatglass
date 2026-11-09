"""Verify selected FFT bin and magnitude are visibly and accessibly reported."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('objectName: "visualizationSpectrumReadout"')
    block = QML[start:start + 1200]
    for marker in ("FFT bin: none selected", "magnitude", "Accessible.role: Accessible.Status", "wheel and keyboard selection update"):
        assert marker in block, marker
    print("Spectrum readout contract verified (4 checks)")

if __name__ == "__main__":
    main()
