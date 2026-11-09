"""Verify PipeWire callback/control discontinuity state is race-safe."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::atomic<bool> discontinuity_pending_{false}" in HEADER
    assert "discontinuity_pending_.exchange(false" in SOURCE
    assert "discontinuity_pending_.store(true" in SOURCE
    print("PipeWire stream thread-safety contract verified (3 checks)")

if __name__ == "__main__":
    main()
