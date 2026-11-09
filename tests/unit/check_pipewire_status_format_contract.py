from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_format() const noexcept" in h
assert "format->sample_rate != 0 && format->channels != 0" in h
print("PipeWire status format contract verified (2 checks)")
