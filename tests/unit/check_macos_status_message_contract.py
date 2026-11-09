from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "Output device or stream disappeared; select it again" in SOURCE
    assert "Output format changed or tap callback failed" in SOURCE
    print("macOS status message contract verified (2 checks)")

if __name__ == "__main__":
    main()
