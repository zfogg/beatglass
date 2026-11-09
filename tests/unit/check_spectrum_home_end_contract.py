"""Verify Home/End navigate the bounded FFT bin selection."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index('Keys.onHomePressed: { if (viewMode === "spectrum")')
    block = QML[start:start + 1000]
    assert 'if (viewMode === "spectrum") selectedSpectrumBin = 0' in block
    assert 'if (viewMode === "spectrum") selectedSpectrumBin = Math.max(0, renderedSpectrum.length - 1)' in block
    print("Spectrum Home/End contract verified (2 checks)")

if __name__ == "__main__":
    main()
