"""Exercise optional device identity validation for PipeWire transitions."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
events = [
    {"event": "start", "epoch": 1, "serial": "sink-a", "device_id": "alsa_input.pci-1"},
    {"event": "process", "epoch": 1, "sample_count": 256, "device_id": "alsa_input.pci-1"},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32, "device_id": "alsa_input.pci-1"},
    {"event": "restart", "epoch": 2, "serial": "sink-b", "device_id": "alsa_input.usb-2", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(items):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": items}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(events).returncode == 0
incomplete = json.loads(json.dumps(events))
del incomplete[2]["device_id"]
result = run(incomplete)
assert result.returncode != 0
assert "complete" in result.stderr
blank = json.loads(json.dumps(events))
blank[1]["device_id"] = "  "
result = run(blank)
assert result.returncode != 0
assert "non-empty" in result.stderr
print("PipeWire device identity behavior verified (3 scenarios)")
