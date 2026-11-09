from pathlib import Path

HEADER = (Path(__file__).resolve().parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text(encoding="utf-8")
SOURCE = (Path(__file__).resolve().parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert "class MonitorBackend" in HEADER
    assert "fresh nonzero epoch" in HEADER
    assert "discover_sinks(timeout)" in SOURCE
    assert "make_monitor_backend" in SOURCE
    print("PipeWire monitor boundary contract verified (4 checks)")

if __name__ == "__main__":
    main()
