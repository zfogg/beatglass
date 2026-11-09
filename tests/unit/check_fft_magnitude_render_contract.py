"""Verify FFT rendering clamps invalid negative magnitudes."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("var spec = renderedSpectrum")
    block = QML[start:start + 1800]
    assert "const magnitude = Math.max(0, spec[k])" in block
    assert "magnitude * height * 2" in block
    print("FFT magnitude render contract verified (2 checks)")

if __name__ == "__main__":
    main()
