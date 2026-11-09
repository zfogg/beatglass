"""Verify the PipeWire backend has a negotiated-format session start seam."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "virtual bool start_stream(const std::string& serial" in HEADER
    assert "session_.start(serial, epoch, sample_rate, channels)" in SOURCE
    assert "session_.stop();" in SOURCE
    print("PipeWire backend session contract verified (3 checks)")

if __name__ == "__main__":
    main()
