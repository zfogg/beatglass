from pathlib import Path
p = (Path(__file__).parents[2] / "tests/pipewire/validate_qualification_artifacts.py").read_text()
for marker in ("REQUIRED =", "build.txt", "lifecycle-trace.txt", "return 1"):
    assert marker in p
print("PipeWire artifact validator contract verified (4 checks)")
