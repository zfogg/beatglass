from pathlib import Path
root = Path(__file__).parents[2]
h = (root / "include/beatglass/capture/detail/pipewire_stream_owner.hpp").read_text()
c = (root / "CMakeLists.txt").read_text()
assert "struct Config" in h
assert "virtual bool connect(const Config& config" in h
assert "virtual bool connected() const noexcept" in h
assert "virtual void stop() noexcept = 0;" in h
assert "lib/capture/pipewire/stream_owner.cpp" in c
source = (root / "lib/capture/pipewire/stream_owner.cpp").read_text()
assert "class UnavailableStreamOwner final" in source
assert "handoff.on_error(-ENOSYS);" in source
linux = (root / "lib/capture/pipewire/stream_owner_linux.cpp").read_text()
for marker in ("pw_thread_loop_start(loop_)", "loop_started_", "pw_thread_loop_stop(loop_)", "pw_stream_connect"):
    assert marker in linux
assert linux.index("pw_thread_loop_stop(loop_)") < linux.index("pw_context_destroy(context_)")
print("PipeWire native stream owner seam verified (7 checks)")
