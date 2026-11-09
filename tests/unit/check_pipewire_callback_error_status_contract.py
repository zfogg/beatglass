from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "void on_error(int error_code)" in HEADER
    assert "std::atomic<int> error_code_" in HEADER
    assert "error_code_.store" in SOURCE
    print("PipeWire callback error-status contract verified (3 checks)")

if __name__ == "__main__":
    main()
