from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool is_consistent() const noexcept" in h
assert "has_context() && has_epoch() && !requires_reacquisition()" in h
print("PipeWire status consistency contract verified (2 checks)")
