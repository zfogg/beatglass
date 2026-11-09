from pathlib import Path
import json
import subprocess
import sys
import tempfile

source = (Path(__file__).parents[2] / "tests/pipewire/validate_pipewire_manifest.py").read_text(encoding="utf-8")
assert "Audio/Sink" in source
for marker in ("schema_version", "captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory", "is_symlink", "refusing symlinked manifest", "must identify Linux", "retain info objects", "retain props objects", "non-negative numeric ids", "object ids must be unique", "unsupported PipeWire qualification manifest schema", "must be a list"):
    assert marker in source
print("PipeWire manifest validator contract verified (6 checks)")

validator = Path(__file__).parents[2] / "tests/pipewire/validate_pipewire_manifest.py"
valid = {"schema_version": 1, "captured_at_utc": "2026-01-01T00:00:00+00:00",
         "host_platform": "Linux", "python_version": "3.12",
         "pipewire_cli_version": "1.0", "audio_sink_count": 1, "inventory": [{"id": 1, "info": {"props": {"media.class": "Audio/Sink", "object.serial": "1", "node.name": "test"}}}]}
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "manifest.json"
    path.write_text(json.dumps(valid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode == 0
    path.write_text(json.dumps({**valid, "schema_version": 99}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "schema_version": True}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "python_version": 312}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1, "info": {"props": {"media.class": "Audio/Sink", "object.serial": "serial", "node.name": "test"}}}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1, "info": {"props": {"media.class": 1}}}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "captured_at_utc": 123}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    duplicate = {"id": 2, "info": {"props": {"media.class": "Audio/Sink", "object.serial": "1", "node.name": "other"}}}
    path.write_text(json.dumps({**valid, "inventory": [valid["inventory"][0], duplicate]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1, "info": {"props": {"media.class": "Audio/Sink"}}}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1, "info": {"props": {"media.class": "Audio/Sink", "object.serial": 1, "node.name": "test"}}}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": ["not-an-object"]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1}, {"id": 1}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": []}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": [{"id": 1, "info": {}}]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "captured_at_utc": "not-a-timestamp"}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "captured_at_utc": "2026-01-01T00:00:00+01:00"}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "host_platform": "Windows"}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
print("PipeWire manifest validator fixture behavior verified (11 scenarios)")
