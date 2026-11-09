"""Verify backend restart forwards fresh epoch/format lifecycle."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "virtual bool restart_stream(std::uint64_t epoch" in HEADER
    assert "bool restart_stream(std::uint64_t epoch" in SOURCE
    assert "session_.restart(epoch, sample_rate, channels)" in SOURCE
    print("PipeWire backend restart contract verified (3 checks)")

if __name__ == "__main__":
    main()
