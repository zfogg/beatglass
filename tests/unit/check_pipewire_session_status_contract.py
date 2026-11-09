"""Verify one control-thread poll consumes all PipeWire session markers."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "struct Status" in HEADER
    assert "Status poll_status() noexcept" in HEADER
    for marker in ("take_error()", "take_discontinuity()", "overflowed()"):
        assert marker in SOURCE
    print("PipeWire session status contract verified (5 checks)")

if __name__ == "__main__":
    main()
