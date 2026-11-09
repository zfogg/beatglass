from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "int take_error() noexcept" in HEADER
    assert "error_code_.exchange(0" in SOURCE
    print("PipeWire callback error-consume contract verified (2 checks)")

if __name__ == "__main__":
    main()
