from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "std::sort(result.begin(), result.end()" in SOURCE
    assert "a.serial < b.serial" in SOURCE
    print("PipeWire sink ordering contract verified (2 checks)")

if __name__ == "__main__":
    main()
