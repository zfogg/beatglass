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
timestamps = [event.get("timestamp_ns") for event in events if isinstance(event, dict) and event.get("timestamp_ns") is not None]
if timestamps and (len(timestamps) != len(events) or any(not isinstance(value, int) or isinstance(value, bool) or value <= 0 for value in timestamps)
                  or timestamps != sorted(timestamps)):
    raise SystemExit("transition timestamps must be positive, complete, and monotonic")
device_ids = [event.get("device_id") for event in events if isinstance(event, dict) and event.get("device_id") is not None]
if device_ids and (len(device_ids) != len(events) or any(not isinstance(value, str) or not value.strip() for value in device_ids)):
    raise SystemExit("transition device identities must be complete and non-empty")
error_event = next(event for event in events if event.get("event") == "error")
if error_event.get("discontinuity") is not True:
    raise SystemExit("transition evidence has no discontinuity marker")
if not isinstance(error_event.get("error_code"), int) or isinstance(error_event.get("error_code"), bool) or error_event["error_code"] == 0:
    raise SystemExit("transition evidence has no native error code")
start = next(event for event in events if event.get("event") == "start")
if not isinstance(start.get("serial"), str) or not start["serial"].strip():
    raise SystemExit("start event has no sink serial")
if not any(isinstance(event.get("sample_count"), int) and not isinstance(event.get("sample_count"), bool) and event["sample_count"] > 0
           and event.get("epoch") == start.get("epoch") for event in events if event.get("event") == "process"):
    raise SystemExit("process evidence has no captured samples")
restart = next(event for event in events if event.get("event") == "restart")
if restart.get("epoch") <= start.get("epoch"):
    raise SystemExit("restart does not advance the capture epoch")
if not isinstance(restart.get("serial"), str) or not restart["serial"].strip():
    raise SystemExit("restart event has no sink serial")
if device_ids and (not isinstance(start.get("device_id"), str) or not isinstance(restart.get("device_id"), str)):
    raise SystemExit("device identity is missing from lifecycle boundary")
reacquisition = restart.get("reacquisition")
if reacquisition is not None:
    if not isinstance(reacquisition, dict) or reacquisition.get("epoch") != restart.get("epoch"):
        raise SystemExit("reacquisition evidence has the wrong epoch")
    if not isinstance(reacquisition.get("sample_count"), int) or isinstance(reacquisition.get("sample_count"), bool) or reacquisition["sample_count"] <= 0:
        raise SystemExit("reacquisition evidence has no captured samples")
    if not isinstance(reacquisition.get("serial"), str) or not reacquisition["serial"].strip():
        raise SystemExit("reacquisition evidence has no sink serial")
fmt = restart.get("format")
if not isinstance(fmt, dict) or fmt.get("sample_rate", 0) <= 0 or fmt.get("channels", 0) <= 0:
    raise SystemExit("restart event has no negotiated format")
for event in events:
    event_format = event.get("format")
    if event_format is not None and (not isinstance(event_format, dict) or event_format.get("sample_rate", 0) <= 0 or event_format.get("channels", 0) <= 0):
        raise SystemExit("transition event has an invalid negotiated format")
    if event_format is not None and event_format != fmt and event.get("event") == "process":
        raise SystemExit("process format disagrees with restart format")
print(f"PipeWire transition evidence verified ({len(events)} events)")
