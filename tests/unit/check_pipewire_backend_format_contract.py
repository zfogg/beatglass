"""Verify negotiated PipeWire format is observable at the backend boundary."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "struct MonitorFormat" in HEADER
    assert "virtual std::optional<MonitorFormat> selected_format() const noexcept = 0" in HEADER
    assert "selected_format() const noexcept override" in SOURCE
    assert "return session_.format();" in SOURCE
    print("PipeWire backend format contract verified (4 checks)")

if __name__ == "__main__":
    main()
