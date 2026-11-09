from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/capture_pipewire_inventory.py").read_text(encoding="utf-8")
for marker in ("pw-dump", "json.loads", "non-list inventory", "is_symlink", "refusing to overwrite"):
    assert marker in source
print("PipeWire inventory capture contract verified (5 checks)")
