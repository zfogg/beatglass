from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "a.serial == b.serial ? a.name < b.name" in SOURCE
    print("PipeWire sink tie-order contract verified (1 check)")

if __name__ == "__main__":
    main()
