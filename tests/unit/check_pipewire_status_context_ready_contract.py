from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool has_context() const noexcept" in h
assert "return has_serial() && has_format()" in h
print("PipeWire status context-ready contract verified (2 checks)")
