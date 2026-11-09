from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "EnumerationOutcome::ok && !result.sources.empty()" in SOURCE
    assert "result.diagnostic.clear()" in SOURCE
    print("macOS enumeration success-clear contract verified (2 checks)")

if __name__ == "__main__":
    main()
