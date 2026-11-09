from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "bool reacquire(const std::string& serial" in c
assert "!owner_->connect({serial, sample_rate, channels}" in c
print("PipeWire native reacquire contract verified (2 checks)")
