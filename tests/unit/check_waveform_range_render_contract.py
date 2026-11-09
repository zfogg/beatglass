"""Verify waveform rendering bounds source PCM extrema to the visible range."""
from pathlib import Path

QML = (Path(__file__).resolve().parents[2] / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")

def main() -> None:
    start = QML.index("var wave = renderedWaveform")
    block = QML[start:start + 1300]
    assert "Math.max(-1, Math.min(1, Number(wave[i][0])))" in block
    assert "Math.max(-1, Math.min(1, Number(wave[i][1])))" in block
    print("Waveform range render contract verified (2 checks)")

if __name__ == "__main__":
    main()
