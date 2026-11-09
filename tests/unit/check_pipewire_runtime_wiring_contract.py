from pathlib import Path
cmake = (Path(__file__).parents[2] / "CMakeLists.txt").read_text()
assert "lib/capture/pipewire/discovery.cpp lib/capture/pipewire/source_adapter.cpp" in cmake
print("PipeWire runtime adapter wiring contract verified")
