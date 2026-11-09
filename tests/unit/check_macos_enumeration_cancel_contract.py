from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "EnumerationOutcome::cancelled" in SOURCE
    assert '"Core Audio output enumeration cancelled"' in SOURCE
    print("macOS enumeration cancellation contract verified (2 checks)")

if __name__ == "__main__":
    main()
