from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_serial() const noexcept" in h
assert "return !serial.empty()" in h
print("PipeWire status serial contract verified (2 checks)")
