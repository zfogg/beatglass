from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool requires_reacquisition() const noexcept" in h
assert "!active || needs_restart || discontinuity || overflowed" in h
print("PipeWire status reacquisition contract verified (2 checks)")
