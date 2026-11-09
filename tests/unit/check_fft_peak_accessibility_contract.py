"""Verify FFT peak data is derived from bounded rendered spectrum values."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("readonly property int spectrumPeakIndex")
    block = QML[start:start + 900]
    assert "renderedSpectrum.length" in block
    assert "isFinite(magnitude)" in block
    assert "peak bin" in QML
    print("FFT peak accessibility contract verified (3 checks)")

if __name__ == "__main__":
    main()
