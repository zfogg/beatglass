"""Source contract for filtering disconnected Core Audio devices."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/device_discovery.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "kAudioDevicePropertyDeviceIsAlive" in SOURCE
    assert "alive_address" in SOURCE
    assert "|| !alive) continue" in SOURCE
    print("macOS live-device filter contract verified (3 checks)")

if __name__ == "__main__":
    main()
