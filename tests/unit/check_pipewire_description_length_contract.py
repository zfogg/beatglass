from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "description.size() > max_identity_length" in SOURCE
    print("PipeWire description length contract verified (1 check)")

if __name__ == "__main__":
    main()
