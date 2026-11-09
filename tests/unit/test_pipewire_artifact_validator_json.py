import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).parents[2]
validator = root / "tests/pipewire/validate_qualification_artifacts.py"
names = ("build.txt", "ctest.txt", "daemon-status.txt", "discovery.txt",
         "stream-format.txt", "callback-trace.txt", "lifecycle-trace.txt", "limitations.txt")
with tempfile.TemporaryDirectory() as directory:
    artifact = Path(directory)
    for name in names:
        (artifact / name).write_text("evidence\n", encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(artifact), "--json"], capture_output=True, text=True, check=True)
    assert result.stdout.startswith('{"artifact_dir":')
    report = json.loads(result.stdout)
    assert report["schema"] == "pipewire-qualification-v1"
    assert report["verified"] is True
    assert set(report["files"]) == set(names)
    incomplete = artifact / "incomplete"
    incomplete.mkdir()
    result = subprocess.run([sys.executable, str(validator), str(incomplete), "--json"])
    assert result.returncode == 1
missing = subprocess.run([sys.executable, str(validator), str(Path(directory) / "missing"), "--json"])
assert missing.returncode == 2
print("PipeWire artifact validator JSON behavior verified")
