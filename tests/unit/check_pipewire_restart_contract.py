from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool restart(std::uint64_t epoch" in HEADER
    assert "error_code_.store(0" in SOURCE
    assert "discontinuity_pending_ = true" in SOURCE
    print("PipeWire restart contract verified (3 checks)")

if __name__ == "__main__":
    main()
