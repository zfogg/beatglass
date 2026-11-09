"""Verify FFT bin selection is cleared on capture/result invalidation."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    assert "visualizationCanvas.selectedSpectrumBin = -1" in QML
    start = QML.index("Connections { target: typeof visualizationPresentation")
    assert "selectedSpectrumBin = -1" in QML[start:start + 500]
    print("Spectrum selection reset contract verified (2 checks)")

if __name__ == "__main__":
    main()
