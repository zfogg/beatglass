from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "bool pop_current(coreaudio::CaptureBlock& block) noexcept;" in h
assert "return pop(block) && accepts_epoch(block.epoch);" in c
assert "return session_.pop_current(block);" in c
print("PipeWire session current-epoch pop contract verified (3 checks)")
