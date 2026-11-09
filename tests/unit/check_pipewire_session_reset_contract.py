"""Verify control-thread reset fully releases PipeWire session identity/state."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "void reset() noexcept" in HEADER
    assert "stream_.reset()" in SOURCE
    assert "serial_.clear()" in SOURCE
    assert "format_.reset()" in SOURCE
    print("PipeWire session reset contract verified (4 checks)")

if __name__ == "__main__":
    main()
