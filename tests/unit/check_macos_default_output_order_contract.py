"""Source contract for macOS default-output ordering."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/device_discovery.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "kAudioHardwarePropertyDefaultOutputDevice" in SOURCE
    assert "kAudioDevicePropertyDeviceUID" in SOURCE
    assert "std::rotate(result.begin(), it, std::next(it))" in SOURCE
    print("macOS default output ordering contract verified (3 checks)")

if __name__ == "__main__":
    main()
