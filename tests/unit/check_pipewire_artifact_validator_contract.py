from pathlib import Path
p = (Path(__file__).parents[2] / "tests/pipewire/validate_qualification_artifacts.py").read_text()
for marker in ("REQUIRED =", "build.txt", "lifecycle-trace.txt", "return 1"):
    assert marker in p
assert "read_text(encoding=\"utf-8\").strip()" in p
assert "if not root.is_dir()" in p
assert "bytes" in p
assert "is_symlink()" in p
assert "--json" in p
assert "json.dumps" in p
assert "artifact_dir" in p
print("PipeWire artifact validator contract verified (4 checks)")
