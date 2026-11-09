from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::string(name).find_first_not_of" in SOURCE
    print("PipeWire sink-name validation contract verified (1 check)")

if __name__ == "__main__":
    main()
