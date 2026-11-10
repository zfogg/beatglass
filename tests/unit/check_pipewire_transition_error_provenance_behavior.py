"""Exercise optional native error-domain and recovery-reason validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
base = [
    {"event": "start", "epoch": 1, "serial": "sink-a"},
    {"event": "process", "epoch": 1, "sample_count": 256},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32, "error_domain": "pipewire"},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "recovery_reason": "stream-reconnected", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(base).returncode == 0
missing_domain = json.loads(json.dumps(base))
missing_domain[2]["error_domain"] = ""
result = run(missing_domain)
assert result.returncode != 0
assert "complete" in result.stderr
missing_reason = json.loads(json.dumps(base))
del missing_reason[3]["recovery_reason"]
result = run(missing_reason)
assert result.returncode != 0
assert "recovery reason" in result.stderr
print("PipeWire error provenance behavior verified (3 scenarios)")
