from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual bool accepts_epoch(std::uint64_t epoch) const noexcept = 0;" in h
assert "epoch != 0 && epoch == epoch_" in h
assert "session_.accepts_epoch(epoch)" in c
print("PipeWire epoch admission contract verified (3 checks)")
