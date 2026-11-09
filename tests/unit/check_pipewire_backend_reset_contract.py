"""Verify backend reset forwards full session teardown."""
from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "virtual void reset() noexcept = 0" in HEADER
    assert "void reset() noexcept override" in SOURCE
    assert "session_.reset()" in SOURCE
    print("PipeWire backend reset contract verified (3 checks)")

if __name__ == "__main__":
    main()
