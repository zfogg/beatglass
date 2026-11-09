from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "owner_(detail::make_stream_owner())" in c
assert "if (owner_) owner_->stop();" in c
print("PipeWire owner reset contract verified (2 checks)")
