from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/pipewire_source.hpp").read_text()
c = (root / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "virtual bool reacquire(const std::string& serial" in h
assert "session_.reset();" in c
assert "if (!valid_serial(serial) || !epoch || !sample_rate || !channels) return false;" in c
assert "if (epoch == session_.epoch()) return false;" in c
assert "session_.start(serial, epoch, sample_rate, channels)" in c
print("PipeWire reacquire contract verified (3 checks)")
