"""Source contract for actionable Core Audio permission errors."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "TapFailure::permission_denied" in SOURCE
    assert "grant macOS audio recording permission and retry" in SOURCE
    assert "std::move(message)" in SOURCE
    print("macOS permission guidance contract verified (3 checks)")

if __name__ == "__main__":
    main()
