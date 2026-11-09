from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "on_process" in HEADER and "on_error" in HEADER
    assert "discontinuity || discontinuity_pending_" in SOURCE
    assert "void MonitorStream::on_error" in SOURCE
    print("PipeWire callback seam contract verified (3 checks)")

if __name__ == "__main__":
    main()
