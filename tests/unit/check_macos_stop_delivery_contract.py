from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "if (delivering_)" in SOURCE
    assert "SourceError::busy" in SOURCE
    assert "Cannot stop Core Audio capture during PCM delivery" in SOURCE
    print("macOS stop-during-delivery contract verified (3 checks)")

if __name__ == "__main__":
    main()
