from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::string(serial).find('\\0')" in SOURCE
    assert "std::string(name).find('\\0')" in SOURCE
    print("PipeWire identity NUL contract verified (2 checks)")

if __name__ == "__main__":
    main()
