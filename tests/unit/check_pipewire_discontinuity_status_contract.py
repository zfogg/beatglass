"""Verify control code can consume PipeWire discontinuity status exactly once."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool take_discontinuity() noexcept" in HEADER
    assert "discontinuity_pending_.exchange(false" in SOURCE
    assert "discontinuity_pending_.store(true" in SOURCE
    print("PipeWire discontinuity status contract verified (3 checks)")

if __name__ == "__main__":
    main()
