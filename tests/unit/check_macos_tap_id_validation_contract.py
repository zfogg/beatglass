from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "uid.find('\\0')" in SOURCE
    assert 'uid.find_first_not_of(" \\t\\r\\n")' in SOURCE
    print("macOS tap ID validation contract verified (2 checks)")

if __name__ == "__main__":
    main()
