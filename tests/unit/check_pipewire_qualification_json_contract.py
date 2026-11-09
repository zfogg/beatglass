from pathlib import Path
p = (Path(__file__).parents[2] / "docs/pipewire-qualification-json.md").read_text()
for marker in ("--json", "verified: true", "files", "byte count", "original evidence"):
    assert marker in p
assert "artifact_dir" in p
assert "Exit code `0`" in p
assert "`2` means" in p
print("PipeWire qualification JSON contract verified (5 checks)")
