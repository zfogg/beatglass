from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "timeout <= std::chrono::milliseconds::zero()" in SOURCE
    assert '"PipeWire discovery timeout must be positive"' in SOURCE
    print("PipeWire timeout contract verified (2 checks)")

if __name__ == "__main__":
    main()
