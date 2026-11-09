"""Emit platform-specific native evidence capture instructions without claiming execution."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: prepare_native_host_capture.py OUTPUT.json")
instructions = {
    "Linux": {"target":"Linux/PipeWire", "status":"pending", "command":"python tests/pipewire/run_linux_native_qualification.py --summary evidence/native-summary.json --archive evidence/native-qualification.zip", "requires":["PipeWire daemon", "libpipewire-0.3", "audio sink"]},
    "Windows": {"target":"Windows/native audio/overlay", "status":"pending", "command":"Run the Windows native qualification checklist and capture summary/archive paths", "requires":["desktop audio permission", "overlay session"]},
    "Darwin": {"target":"macOS/native audio/overlay", "status":"pending", "command":"Run the macOS native qualification checklist and capture summary/archive paths", "requires":["Screen Recording permission", "audio capture permission", "overlay session"]},
}
output = Path(sys.argv[1]); output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps({"schema":"beatglass-native-host-capture-plan-v1", "fabricated":False, "platforms":instructions}, indent=2) + "\n", encoding="utf-8")
print(f"Wrote pending native host capture plan to {output}")
