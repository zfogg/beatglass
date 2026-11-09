from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool usable() const noexcept" in h
assert "active && !requires_reacquisition() && !has_error()" in h
print("PipeWire status usable contract verified (2 checks)")
