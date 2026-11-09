"""Source contract for typed macOS device-loss interruption."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "const bool device_lost = alive_status != noErr || !alive" in SOURCE
    assert "device_lost ? TapFailure::device_lost : TapFailure::backend_error" in SOURCE
    assert "TapState::interrupted" in SOURCE
    print("macOS device-loss classification contract verified (3 checks)")

if __name__ == "__main__":
    main()
