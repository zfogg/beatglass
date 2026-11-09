from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool epoch_matches(std::uint64_t expected) const noexcept" in h
assert "expected != 0 && epoch == expected" in h
print("PipeWire status epoch-match contract verified (2 checks)")
