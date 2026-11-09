from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/discovery.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "pw_init(nullptr, nullptr);" in SOURCE
    assert "struct PwShutdown" in SOURCE
    assert "pw_deinit();" in SOURCE
    print("PipeWire shutdown contract verified (3 checks)")

if __name__ == "__main__":
    main()
