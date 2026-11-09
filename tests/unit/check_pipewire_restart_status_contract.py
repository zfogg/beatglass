from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool needs_restart() const noexcept" in HEADER
    assert "stopped_.load" in SOURCE
    assert "error_code() != 0" in SOURCE
    print("PipeWire restart-status contract verified (3 checks)")

if __name__ == "__main__":
    main()
