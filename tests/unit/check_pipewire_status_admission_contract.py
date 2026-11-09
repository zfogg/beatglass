from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool accepts_epoch(std::uint64_t candidate) const noexcept" in h
assert "active && candidate != 0 && candidate == epoch" in h
print("PipeWire status admission contract verified (2 checks)")
