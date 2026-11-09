from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_boundary() const noexcept" in h
assert "discontinuity || overflowed || needs_restart" in h
print("PipeWire status boundary contract verified (2 checks)")
