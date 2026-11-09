"""Validate a daemon-captured PipeWire lifecycle transition JSON artifact."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_pipewire_transition_evidence.py EVENTS.json")
payload = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
events = payload.get("events") if isinstance(payload, dict) else payload
if isinstance(payload, dict):
    if payload.get("schema") != "beatglass-pipewire-transitions-v1" or not str(payload.get("commit", "")).strip():
        raise SystemExit("transition evidence has incomplete provenance")
if not isinstance(events, list) or not events:
    raise SystemExit("transition evidence must be a non-empty event list")
if any(not isinstance(event, dict) for event in events):
    raise SystemExit("transition evidence contains a non-object event")
names = [event.get("event") for event in events if isinstance(event, dict)]
if any(not isinstance(name, str) or not name.strip() for name in names):
    raise SystemExit("transition evidence contains an unnamed event")
required = ("start", "process", "error", "restart")
missing = [name for name in required if name not in names]
if missing:
    raise SystemExit(f"transition evidence missing: {', '.join(missing)}")
positions = [names.index(name) for name in required]
if positions != sorted(positions):
    raise SystemExit("transition lifecycle events are out of order")
epochs = [event.get("epoch") for event in events if isinstance(event, dict) and event.get("epoch") is not None]
if any(not isinstance(epoch, int) or isinstance(epoch, bool) or epoch <= 0 for epoch in epochs) or epochs != sorted(epochs):
    raise SystemExit("transition epochs must be positive and monotonic")
if not any(event.get("discontinuity") is True for event in events if isinstance(event, dict)):
    raise SystemExit("transition evidence has no discontinuity marker")
if not any(event.get("error_code", 0) != 0 for event in events if isinstance(event, dict)):
    raise SystemExit("transition evidence has no native error code")
start = next(event for event in events if event.get("event") == "start")
if not isinstance(start.get("serial"), str) or not start["serial"].strip():
    raise SystemExit("start event has no sink serial")
if not any(isinstance(event.get("sample_count"), int) and event["sample_count"] > 0
           for event in events if event.get("event") == "process"):
    raise SystemExit("process evidence has no captured samples")
restart = next(event for event in events if event.get("event") == "restart")
fmt = restart.get("format")
if not isinstance(fmt, dict) or fmt.get("sample_rate", 0) <= 0 or fmt.get("channels", 0) <= 0:
    raise SystemExit("restart event has no negotiated format")
print(f"PipeWire transition evidence verified ({len(events)} events)")
