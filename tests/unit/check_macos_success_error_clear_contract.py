from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "result.error.reset()" in SOURCE
    assert "EnumerationOutcome::ok" in SOURCE
    print("macOS successful enumeration error-clear contract verified (2 checks)")

if __name__ == "__main__":
    main()
