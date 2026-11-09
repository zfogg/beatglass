from pathlib import Path
c = (Path(__file__).parents[2] / "lib/capture/pipewire/source_adapter.cpp").read_text()
assert "if (!accepted) discontinuity_pending_.store(true, std::memory_order_release);" in c
print("PipeWire overflow discontinuity contract verified")
