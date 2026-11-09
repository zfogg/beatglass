"""Exercise deterministic native evidence archive creation."""
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

packager = Path(__file__).parents[1] / "pipewire" / "package_native_evidence.py"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    discovery = root / "pipewire-tests-discovery.json"
    junit = root / "pipewire-tests.xml"
    discovery.write_text('{"tests": [{"name": "capture"}]}\n', encoding="utf-8")
    junit.write_text("<testsuite tests='1'/>\n", encoding="utf-8")
    summary = root / "native-summary.json"
    summary.write_text(json.dumps({"qualified": True, "commit": "fixture-commit", "failed_command": None, "native_discovery_json": str(discovery), "native_junit": str(junit), "native_discovery_sha256": hashlib.sha256(discovery.read_bytes()).hexdigest(), "native_junit_sha256": hashlib.sha256(junit.read_bytes()).hexdigest(), "commands": []}), encoding="utf-8")
    env = dict(os.environ, SOURCE_DATE_EPOCH="1700000000")
    for name in ("first.zip", "second.zip"):
        result = subprocess.run([sys.executable, str(packager), str(summary), str(root / name)], env=env, capture_output=True, text=True)
        if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)
    if (root / "first.zip").read_bytes() != (root / "second.zip").read_bytes(): raise SystemExit("fixed SOURCE_DATE_EPOCH did not produce reproducible ZIP bytes")
print("PipeWire evidence packager behavior passed (1 reproducibility scenario)")
