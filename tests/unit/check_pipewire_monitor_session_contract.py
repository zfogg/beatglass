"""Verify the control-thread PipeWire monitor session boundary."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "class MonitorSession" in HEADER
    assert "bool pop_current(coreaudio::CaptureBlock& block) noexcept;" in HEADER
    assert "if (!valid_serial(serial) || !epoch || !sample_rate || !channels)" in SOURCE
    assert "stream_->on_process" in SOURCE
    assert "stream_->restart" in SOURCE
    print("PipeWire monitor session contract verified (4 checks)")

if __name__ == "__main__":
    main()
