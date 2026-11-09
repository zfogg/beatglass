from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual std::uint64_t selected_epoch() const noexcept = 0;" in h
assert "selected_epoch() const noexcept override { return session_.epoch(); }" in c
print("PipeWire selected epoch contract verified (2 checks)")
