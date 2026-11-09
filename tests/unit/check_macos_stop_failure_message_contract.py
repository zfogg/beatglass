from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert '"Core Audio cleanup failed; retry stop before restart"' in SOURCE
    assert "TapFailure::backend_error" in SOURCE
    print("macOS stop-failure message contract verified (2 checks)")

if __name__ == "__main__":
    main()
