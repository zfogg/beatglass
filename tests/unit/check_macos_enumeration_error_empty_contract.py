from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "result.outcome == EnumerationOutcome::error" in SOURCE
    assert "result.sources.clear()" in SOURCE
    assert "Core Audio output enumeration failed" in SOURCE
    print("macOS enumeration error-empty contract verified (3 checks)")

if __name__ == "__main__":
    main()
