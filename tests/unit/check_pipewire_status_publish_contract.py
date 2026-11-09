from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool can_publish() const noexcept" in h
assert "is_consistent() && !has_error()" in h
print("PipeWire status publish contract verified (2 checks)")
