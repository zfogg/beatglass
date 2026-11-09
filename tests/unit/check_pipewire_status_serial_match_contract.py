from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool serial_matches(const std::string& expected) const noexcept" in h
assert "!expected.empty() && serial == expected" in h
print("PipeWire status serial-match contract verified (2 checks)")
