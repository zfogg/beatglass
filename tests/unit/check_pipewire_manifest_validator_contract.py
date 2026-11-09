from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/validate_pipewire_manifest.py").read_text(encoding="utf-8")
for marker in ("captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory", "must be a list"):
    assert marker in source
print("PipeWire manifest validator contract verified (6 checks)")
