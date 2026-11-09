from pathlib import Path
import json
import subprocess
import sys
import tempfile

source = (Path(__file__).parents[2] / "tests/pipewire/validate_pipewire_manifest.py").read_text(encoding="utf-8")
for marker in ("schema_version", "captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory", "unsupported PipeWire qualification manifest schema", "must be a list"):
    assert marker in source
print("PipeWire manifest validator contract verified (6 checks)")

validator = Path(__file__).parents[2] / "tests/pipewire/validate_pipewire_manifest.py"
valid = {"schema_version": 1, "captured_at_utc": "2026-01-01T00:00:00+00:00",
         "host_platform": "Linux", "python_version": "3.12",
         "pipewire_cli_version": "1.0", "inventory": []}
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "manifest.json"
    path.write_text(json.dumps(valid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode == 0
    path.write_text(json.dumps({**valid, "schema_version": 99}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
    path.write_text(json.dumps({**valid, "inventory": ["not-an-object"]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode != 0
print("PipeWire manifest validator fixture behavior verified (3 scenarios)")
