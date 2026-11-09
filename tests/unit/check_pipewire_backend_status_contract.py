"""Verify backend status forwarding is a shared capture boundary."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "struct MonitorStatus" in HEADER
    assert "virtual MonitorStatus poll_status() noexcept" in HEADER
    assert "MonitorStatus poll_status() noexcept override" in SOURCE
    assert "session_.poll_status()" in SOURCE
    print("PipeWire backend status contract verified (4 checks)")

if __name__ == "__main__":
    main()
