from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/detail/pipewire_stream_owner.hpp").read_text()
c = (root / "CMakeLists.txt").read_text()
assert "virtual bool connect(const std::string& serial" in h
assert "virtual void stop() noexcept = 0;" in h
assert "lib/capture/pipewire/stream_owner.cpp" in c
print("PipeWire native stream owner seam verified (3 checks)")
