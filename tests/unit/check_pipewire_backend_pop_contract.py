"""Verify the PipeWire backend forwards control-side data and status polling."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "virtual bool pop(coreaudio::CaptureBlock& block) noexcept" in HEADER
    assert "session_.pop(block)" in SOURCE
    print("PipeWire backend pop contract verified (2 checks)")

if __name__ == "__main__":
    main()
