from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "if (!stream_ || !stream_->needs_restart()) return false;" in c
assert "if (!needs_restart()) return false;" in c
assert "if (!epoch || epoch == epoch_) return false;" in c
print("PipeWire restart guard contract verified")
