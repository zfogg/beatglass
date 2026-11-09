"""Verify status polling carries source identity and negotiated format."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::string serial" in HEADER
    assert "std::optional<MonitorFormat> format" in HEADER
    assert "status.serial = serial_" in SOURCE
    assert "status.format = format_" in SOURCE
    print("PipeWire status context contract verified (4 checks)")

if __name__ == "__main__":
    main()
