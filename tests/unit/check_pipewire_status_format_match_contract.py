from pathlib import Path
h = (Path(__file__).parents[2] / "include/beatglass/capture/pipewire_source.hpp").read_text()
assert "bool format_matches(std::uint32_t sample_rate, std::uint32_t channels) const noexcept" in h
assert "format->sample_rate == sample_rate && format->channels == channels" in h
print("PipeWire status format-match contract verified (2 checks)")
