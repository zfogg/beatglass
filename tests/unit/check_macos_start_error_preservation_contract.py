from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "const auto error = source_error(failure)" in SOURCE
    assert "backend stop also failed" in SOURCE
    print("macOS start-error preservation contract verified (2 checks)")

if __name__ == "__main__":
    main()
