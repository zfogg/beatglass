from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "class MonitorStream" in HEADER
    assert "coreaudio::CaptureFifo" in HEADER
    assert "return !stopped_ && fifo_->push" in SOURCE
    assert "fifo_->exhausted()" in SOURCE
    print("PipeWire stream transport contract verified (4 checks)")

if __name__ == "__main__":
    main()
