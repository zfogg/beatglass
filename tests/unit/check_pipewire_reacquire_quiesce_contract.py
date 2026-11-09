from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "if (epoch == session_.epoch()) return false;" in c
assert "if (owner_) owner_->stop();" in c
print("PipeWire reacquire quiesce contract verified")
