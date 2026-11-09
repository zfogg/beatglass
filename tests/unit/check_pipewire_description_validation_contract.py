from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "description.find_first_not_of" in SOURCE
    assert "? name : description" in SOURCE
    print("PipeWire description validation contract verified (2 checks)")

if __name__ == "__main__":
    main()
