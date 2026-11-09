"""Verify status context carries the active capture epoch."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::uint64_t epoch{}" in HEADER
    assert "epoch_ = epoch" in SOURCE
    assert "status.epoch = epoch_" in SOURCE
    assert "epoch_ = 0" in SOURCE
    print("PipeWire status epoch contract verified (4 checks)")

if __name__ == "__main__":
    main()
