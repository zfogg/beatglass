from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_error() const noexcept" in h
assert "return error_code != 0" in h
print("PipeWire status error contract verified (2 checks)")
