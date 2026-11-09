from pathlib import Path

root = Path(__file__).parents[2]
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
assert "lib/capture/pipewire/stream_owner_linux.cpp" in cmake
assert "pipewire_linux_native_prerequisites" in cmake
assert "BEATGLASS_BUILD_PIPEWIRE" in cmake
print("PipeWire native build contract verified (3 checks)")
