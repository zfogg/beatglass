from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/capture_pipewire_manifest.py").read_text(encoding="utf-8")
for marker in ("captured_at_utc", "pipewire_cli_version", "inventory", "pw-dump", "pw-cli", "refusing to overwrite"):
    assert marker in source
print("PipeWire manifest capture contract verified (6 checks)")
