from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_epoch() const noexcept" in h
assert "return epoch != 0" in h
print("PipeWire status has-epoch contract verified (2 checks)")
