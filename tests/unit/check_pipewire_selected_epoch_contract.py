from pathlib import Path

ROOT = Path(__file__).parents[2]
header = (ROOT / "include/beatglass/capture/pipewire_source.hpp").read_text()
adapter = (ROOT / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual std::uint64_t selected_epoch() const noexcept = 0;" in header
assert "std::uint64_t selected_epoch() const noexcept override { return session_.epoch(); }" in adapter
assert "or zero when stopped" in header
print("PipeWire selected epoch contract verified (3 checks)")
