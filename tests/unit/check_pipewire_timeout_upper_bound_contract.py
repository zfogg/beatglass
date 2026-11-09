from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::numeric_limits<int>::max()" in SOURCE
    assert '"PipeWire discovery timeout is too large"' in SOURCE
    print("PipeWire timeout upper-bound contract verified (2 checks)")

if __name__ == "__main__":
    main()
