from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "if (!session_.restart(epoch, sample_rate, channels)) return false;" in c
assert "owner_->connect(session_.serial(), sample_rate, channels" in c
print("PipeWire native restart contract verified (2 checks)")
