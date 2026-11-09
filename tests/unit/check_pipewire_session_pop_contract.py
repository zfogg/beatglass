"""Verify the monitor session exposes control-side FIFO consumption."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool pop(coreaudio::CaptureBlock& block) noexcept" in HEADER
    assert "return stream_ && stream_->pop(block);" in SOURCE
    assert "MonitorSession::on_process" in SOURCE
    print("PipeWire session pop contract verified (3 checks)")

if __name__ == "__main__":
    main()
