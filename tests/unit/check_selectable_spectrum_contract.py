"""Verify FFT bins are selectable with pointer and keyboard interaction."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('property int selectedSpectrumBin')
    block = QML[start:start + 15000]
    for marker in ("selectedSpectrumBin = Math.max", "selectedSpectrumBin = Math.min", "Math.floor(mouse.x * visualizationCanvas.renderedSpectrum.length /", "selected bin"):
        assert marker in block, marker
    print("Selectable spectrum contract verified (4 checks)")

if __name__ == "__main__":
    main()
