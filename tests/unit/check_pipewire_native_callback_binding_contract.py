from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/detail/pipewire_native_callbacks.hpp").read_text()
assert "struct NativeCallbackBinding" in h
assert "handoff->on_process" in h
assert "handoff->on_error(error_code)" in h
print("PipeWire native callback binding contract verified (3 checks)")
