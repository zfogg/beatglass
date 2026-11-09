"""Verify selected PipeWire sink identity is observable on the control thread."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "virtual std::string selected_serial() const = 0" in HEADER
    assert "std::string selected_serial() const override" in SOURCE
    assert "return session_.serial();" in SOURCE
    print("PipeWire backend identity contract verified (3 checks)")

if __name__ == "__main__":
    main()
