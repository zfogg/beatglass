from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert 'id.find_first_not_of(" \\t\\r\\n")' in SOURCE
    assert 'SourceError::not_found, "Output source id required"' in SOURCE
    print("macOS source-id whitespace contract verified (2 checks)")

if __name__ == "__main__":
    main()
