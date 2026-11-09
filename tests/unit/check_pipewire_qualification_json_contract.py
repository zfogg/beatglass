from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-qualification-json.md").read_text()
for marker in ("--json", "verified: true", "files", "byte count", "original evidence"):
    assert marker in p
print("PipeWire qualification JSON contract verified (5 checks)")
