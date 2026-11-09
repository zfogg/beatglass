from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/capture_pipewire_manifest.py").read_text(encoding="utf-8")
for marker in ("schema_version", "captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "audio_sink_count", "inventory_sha256", "Audio/Sink", "inventory", "pw-dump", "pw-cli", "NamedTemporaryFile", "os.link", "refusing to overwrite"):
    assert marker in source
print("PipeWire manifest capture contract verified (6 checks)")
