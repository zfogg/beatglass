from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert c.count("if (owner_) owner_->stop();") >= 4
print("PipeWire native failure quiesce contract verified")
