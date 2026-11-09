from pathlib import Path
root = Path(__file__).parents[2]
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "owner_->connect({serial, sample_rate, channels}" in c
assert "session_.reset();" in c
print("PipeWire native connect contract verified (2 checks)")
