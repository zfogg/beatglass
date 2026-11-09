from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/detail/pipewire_stream_owner.hpp").read_text()
c = (root / "CMakeLists.txt").read_text()
assert "virtual bool connect(const std::string& serial" in h
assert "virtual void stop() noexcept = 0;" in h
assert "lib/capture/pipewire/stream_owner.cpp" in c
source = (root / "lib/capture/pipewire/stream_owner.cpp").read_text()
assert "class UnavailableStreamOwner final" in source
assert "handoff.on_error(-ENOSYS);" in source
print("PipeWire native stream owner seam verified (3 checks)")
