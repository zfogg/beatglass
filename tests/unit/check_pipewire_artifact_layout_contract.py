from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-linux-artifact-layout.md").read_text()
for marker in ("build.txt", "ctest.txt", "callback-trace.txt", "lifecycle-trace.txt", "source reacquisition"):
    assert marker in p
print("PipeWire artifact layout contract verified (5 checks)")
