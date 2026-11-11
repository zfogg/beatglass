from pathlib import Path

root = Path(__file__).parents[2]
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
assert "lib/capture/pipewire/stream_owner_linux.cpp" in cmake
assert "pipewire_linux_native_prerequisites" in cmake
assert "BEATGLASS_BUILD_PIPEWIRE" in cmake
assert "_beatglass_pipewire_default" in cmake
assert "pkg_check_modules(_BEATGLASS_PIPEWIRE_PROBE QUIET libpipewire-0.3)" in cmake
readme = (root / "README.md").read_text(encoding="utf-8")
assert "automatically enables PipeWire" in readme
print("PipeWire native build contract verified (6 checks)")
