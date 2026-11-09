from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "bool stop() noexcept override" in c
assert "if (owner_) owner_->stop();" in c
assert "session_.stop();" in c
print("PipeWire owner stop contract verified (3 checks)")
