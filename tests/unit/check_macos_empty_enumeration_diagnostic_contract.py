from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "EnumerationOutcome::ok && result.sources.empty()" in SOURCE
    assert '"No Core Audio output devices available"' in SOURCE
    print("macOS empty enumeration diagnostic contract verified (2 checks)")

if __name__ == "__main__":
    main()
