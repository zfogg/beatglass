from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-linux-qualification-matrix.md").read_text()
for marker in ("Discovery", "Start/format", "Process", "Error", "Stop", "Restart", "Reacquire", "artifact path"):
    assert marker in p
print("PipeWire qualification matrix verified (8 checks)")
