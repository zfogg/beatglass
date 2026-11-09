from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert 'SourceError::not_found, "Output source id required"' in SOURCE
    assert 'SourceError::backend_error, "Audio consumer required"' in SOURCE
    print("macOS invalid-source contract verified (2 checks)")

if __name__ == "__main__":
    main()
