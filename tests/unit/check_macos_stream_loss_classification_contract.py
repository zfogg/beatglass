"""Source contract for Core Audio stream-loss classification."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "const bool stream_lost = !first_output_stream" in SOURCE
    assert "(device_lost || stream_lost) ? TapFailure::device_lost" in SOURCE
    assert "TapState::interrupted" in SOURCE
    print("macOS stream-loss classification contract verified (3 checks)")

if __name__ == "__main__":
    main()
