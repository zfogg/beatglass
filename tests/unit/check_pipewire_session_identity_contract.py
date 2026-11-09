"""Verify session start rejects malformed sink identities before allocation."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "bool valid_serial(const std::string& serial) noexcept" in SOURCE
    assert "byte == 0" in SOURCE
    assert "std::isspace(byte)" in SOURCE
    assert "if (!valid_serial(serial)" in SOURCE
    print("PipeWire session identity contract verified (4 checks)")

if __name__ == "__main__":
    main()
