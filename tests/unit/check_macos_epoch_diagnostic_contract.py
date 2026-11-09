from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert '"Output UID required"' in SOURCE
    assert '"Nonzero capture epoch required"' in SOURCE
    print("macOS epoch diagnostic contract verified (2 checks)")

if __name__ == "__main__":
    main()
