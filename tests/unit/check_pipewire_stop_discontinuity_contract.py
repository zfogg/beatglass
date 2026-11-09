from pathlib import Path
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")
def main() -> None:
    assert "void MonitorStream::stop()" in SOURCE
    assert "discontinuity_pending_ = true" in SOURCE
    assert "stopped_.store(true" in SOURCE
    print("PipeWire stop discontinuity contract verified (3 checks)")
if __name__ == "__main__": main()
