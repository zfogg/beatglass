from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual bool pop_current(coreaudio::CaptureBlock& block) noexcept = 0;" in h
assert "session_.pop(block) && session_.accepts_epoch(block.epoch)" in c
print("PipeWire current-epoch pop contract verified (2 checks)")
