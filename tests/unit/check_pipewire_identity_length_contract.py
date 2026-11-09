from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "max_identity_length = 4096" in SOURCE
    assert "size() > max_identity_length" in SOURCE
    print("PipeWire identity length contract verified (2 checks)")

if __name__ == "__main__":
    main()
