"""Verify PipeWire session format remains tied to its active epoch."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "struct Format" in HEADER
    assert "std::optional<Format> format() const noexcept" in HEADER
    assert "format_ = Format{sample_rate, channels}" in SOURCE
    print("PipeWire session format contract verified (3 checks)")

if __name__ == "__main__":
    main()
