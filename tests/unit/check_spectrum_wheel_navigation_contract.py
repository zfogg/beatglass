"""Verify mouse-wheel navigation moves selected FFT bins with modifiers."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("const binStep")
    block = QML[start:start + 700]
    assert "selectedSpectrumBin" in block
    assert "Qt.ControlModifier" in block and "Qt.ShiftModifier" in block
    assert "Math.sign(wheel.angleDelta.y)" in block
    print("Spectrum wheel navigation contract verified (3 checks)")

if __name__ == "__main__":
    main()
