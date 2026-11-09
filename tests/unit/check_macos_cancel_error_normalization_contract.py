from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "result.error = SourceError::cancelled;" in SOURCE
    assert "result.outcome == EnumerationOutcome::cancelled" in SOURCE
    print("macOS cancellation error normalization contract verified (2 checks)")

if __name__ == "__main__":
    main()
