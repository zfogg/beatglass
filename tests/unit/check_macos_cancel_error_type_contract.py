from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "result.error = SourceError::cancelled" in SOURCE
    assert "EnumerationOutcome::cancelled" in SOURCE
    print("macOS cancelled enumeration error-type contract verified (2 checks)")

if __name__ == "__main__":
    main()
