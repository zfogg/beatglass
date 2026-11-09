from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual bool selected_active() const noexcept = 0;" in h
assert "bool active() const noexcept" in h
assert "session_.active()" in c
print("PipeWire selected active contract verified (3 checks)")
