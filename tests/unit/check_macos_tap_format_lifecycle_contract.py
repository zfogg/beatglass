"""Source contract for complete Core Audio format drift detection."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/coreaudio/output_tap.mm").read_text(encoding="utf-8")

def main() -> None:
    assert "format.mFramesPerPacket != impl_->format.mFramesPerPacket" in SOURCE
    assert "format.mBytesPerPacket != impl_->format.mBytesPerPacket" in SOURCE
    assert "TapState::interrupted" in SOURCE
    print("macOS tap format lifecycle contract verified (3 checks)")

if __name__ == "__main__":
    main()
