"""Verify stopped/error sessions are not reported as actively tracking."""
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "status.active = static_cast<bool>(stream_) && !stream_->needs_restart();" in SOURCE
    print("PipeWire active-status contract verified (1 check)")

if __name__ == "__main__":
    main()
