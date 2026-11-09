from pathlib import Path
c = (Path(__file__).parents[2] / "CMakeLists.txt").read_text()
assert "check_include_file_cxx(\"pipewire/pipewire.h\" BEATGLASS_HAS_PIPEWIRE_HEADER)" in c
assert "BEATGLASS_HAS_PIPEWIRE=1" in c
print("PipeWire header guard contract verified (2 checks)")
