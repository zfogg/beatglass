"""Verify PageUp/PageDown move bounded FFT selection by useful steps."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("Keys.onPageUpPressed")
    block = QML[start:start + 1000]
    assert 'if (viewMode === "spectrum") selectedSpectrumBin = Math.max(0' in block
    assert 'selectedSpectrumBin) - 10' in block
    assert 'selectedSpectrumBin) + 10' in block
    print("Spectrum Page navigation contract verified (3 checks)")

if __name__ == "__main__":
    main()
