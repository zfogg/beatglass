from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "case TapFailure::invalid_argument: return SourceError::not_found;" in SOURCE
    print("macOS invalid-argument mapping contract verified (1 check)")

if __name__ == "__main__":
    main()
