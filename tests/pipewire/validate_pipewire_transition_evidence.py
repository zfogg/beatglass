"""Validate a daemon-captured PipeWire lifecycle transition JSON artifact."""
import json
import re
import sys
from datetime import datetime
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_pipewire_transition_evidence.py EVENTS.json")
payload = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
events = payload.get("events") if isinstance(payload, dict) else payload
if isinstance(payload, dict):
    if payload.get("schema") != "beatglass-pipewire-transitions-v1" or not str(payload.get("commit", "")).strip():
        raise SystemExit("transition evidence has incomplete provenance")
    host = payload.get("host")
    if host is not None:
        if not isinstance(host, dict) or host.get("platform") not in ("linux", "windows", "macos"):
            raise SystemExit("transition host provenance has an invalid platform")
        if not isinstance(host.get("daemon_version"), str) or not host["daemon_version"].strip():
            raise SystemExit("transition host provenance has no daemon version")
        if not isinstance(host.get("capture_command"), str) or not host["capture_command"].strip():
            raise SystemExit("transition host provenance has no capture command")
    if payload.get("artifact_id") is not None and (not isinstance(payload["artifact_id"], str) or not payload["artifact_id"].strip()):
        raise SystemExit("transition evidence has an empty artifact identity")
    if payload.get("artifact_sha256") is not None and (not isinstance(payload["artifact_sha256"], str) or not re.fullmatch(r"[0-9a-fA-F]{64}", payload["artifact_sha256"])):
        raise SystemExit("transition evidence has an invalid artifact digest")
    capture_started_at = None
    if payload.get("capture_started_at") is not None:
        if not isinstance(payload["capture_started_at"], str):
            raise SystemExit("transition evidence has an invalid capture timestamp")
        try:
            capture_started_at = datetime.fromisoformat(payload["capture_started_at"].replace("Z", "+00:00"))
        except ValueError:
            raise SystemExit("transition evidence has an invalid capture timestamp")
        if capture_started_at.tzinfo is None:
            raise SystemExit("transition capture timestamp has no timezone")
    if payload.get("capture_ended_at") is not None:
        if not isinstance(payload["capture_ended_at"], str):
            raise SystemExit("transition evidence has an invalid capture end timestamp")
        try:
            capture_ended_at = datetime.fromisoformat(payload["capture_ended_at"].replace("Z", "+00:00"))
        except ValueError:
            raise SystemExit("transition evidence has an invalid capture end timestamp")
        if capture_ended_at.tzinfo is None:
            raise SystemExit("transition capture end timestamp has no timezone")
        if capture_started_at is not None and capture_ended_at < capture_started_at:
            raise SystemExit("transition capture end precedes capture start")
if not isinstance(events, list) or not events:
    raise SystemExit("transition evidence must be a non-empty event list")
if isinstance(payload, dict) and payload.get("event_count") is not None:
    if not isinstance(payload["event_count"], int) or isinstance(payload["event_count"], bool) or payload["event_count"] != len(events):
        raise SystemExit("transition event count does not match artifact")
if any(not isinstance(event, dict) for event in events):
    raise SystemExit("transition evidence contains a non-object event")
names = [event.get("event") for event in events if isinstance(event, dict)]
if any(not isinstance(name, str) or not name.strip() for name in names):
    raise SystemExit("transition evidence contains an unnamed event")
required = ("start", "process", "error", "restart")
missing = [name for name in required if name not in names]
if missing:
    raise SystemExit(f"transition evidence missing: {', '.join(missing)}")
duplicates = [name for name in ("start", "error", "restart") if names.count(name) != 1]
if duplicates:
    raise SystemExit(f"transition lifecycle events must be unique: {', '.join(duplicates)}")
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
positions = [event.get("sample_position") for event in events if isinstance(event, dict) and event.get("sample_position") is not None]
if positions and (len(positions) != len(events) or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in positions)
                 or positions != sorted(positions)):
    raise SystemExit("transition sample positions must be complete and monotonic")
sequences = [event.get("sequence") for event in events if isinstance(event, dict) and event.get("sequence") is not None]
if sequences and (len(sequences) != len(events) or sequences != list(range(len(events)))
                 or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in sequences)):
    raise SystemExit("transition event sequences must be complete and contiguous")
start = next(event for event in events if event.get("event") == "start")
error_event = next(event for event in events if event.get("event") == "error")
if error_event.get("discontinuity") is not True:
    raise SystemExit("transition evidence has no discontinuity marker")
if error_event.get("epoch") != start.get("epoch"):
    raise SystemExit("error event does not belong to the active capture epoch")
if not isinstance(error_event.get("error_code"), int) or isinstance(error_event.get("error_code"), bool) or error_event["error_code"] == 0:
    raise SystemExit("transition evidence has no native error code")
if not isinstance(start.get("serial"), str) or not start["serial"].strip():
    raise SystemExit("start event has no sink serial")
if error_event.get("serial") is not None and error_event.get("serial") != start.get("serial"):
    raise SystemExit("error event serial does not match active sink")
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
error_events = [event for event in events if event.get("event") == "error"]
error_domains = [event.get("error_domain") for event in error_events]
if any(event.get("error_domain") is not None for event in error_events) and (any(not isinstance(value, str) or not value.strip() for value in error_domains)):
    raise SystemExit("transition error domains must be complete and non-empty")
if any(event.get("error_domain") is not None for event in error_events) and (not isinstance(restart.get("recovery_reason"), str) or not restart.get("recovery_reason", "").strip()):
    raise SystemExit("recovery reason is missing from native error evidence")
reacquisition = restart.get("reacquisition")
if reacquisition is not None:
    if not isinstance(reacquisition, dict) or reacquisition.get("epoch") != restart.get("epoch"):
        raise SystemExit("reacquisition evidence has the wrong epoch")
    if not isinstance(reacquisition.get("sample_count"), int) or isinstance(reacquisition.get("sample_count"), bool) or reacquisition["sample_count"] <= 0:
        raise SystemExit("reacquisition evidence has no captured samples")
    if not isinstance(reacquisition.get("serial"), str) or not reacquisition["serial"].strip():
        raise SystemExit("reacquisition evidence has no sink serial")
    restart_index = events.index(restart)
    if not any(event.get("event") == "process" and event.get("epoch") == restart.get("epoch")
               and isinstance(event.get("sample_count"), int) and not isinstance(event.get("sample_count"), bool)
               and event["sample_count"] > 0 for event in events[restart_index + 1:]):
        raise SystemExit("reacquisition evidence has no post-restart samples")
fmt = restart.get("format")
if not isinstance(fmt, dict) or fmt.get("sample_rate", 0) <= 0 or fmt.get("channels", 0) <= 0:
    raise SystemExit("restart event has no negotiated format")
for event in events:
    event_format = event.get("format")
    if event_format is not None and (not isinstance(event_format, dict) or event_format.get("sample_rate", 0) <= 0 or event_format.get("channels", 0) <= 0):
        raise SystemExit("transition event has an invalid negotiated format")
    if event_format is not None and event_format != fmt and event.get("event") == "process":
        raise SystemExit("process format disagrees with restart format")
process_positions = [(event.get("sample_position"), event.get("sample_count")) for event in events if event.get("event") == "process" and event.get("sample_position") is not None]
for previous, current in zip(process_positions, process_positions[1:]):
    if previous[1] is not None and current[0] < previous[0] + previous[1]:
        raise SystemExit("process sample positions overlap captured samples")
process_events = [event for event in events if event.get("event") == "process"]
sample_counts = [event.get("sample_count") for event in process_events if event.get("sample_count") is not None]
if sample_counts and (len(sample_counts) != len(process_events)
                      or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in sample_counts)):
    raise SystemExit("process sample counts must be complete and non-negative")
drop_counts = [event.get("drop_count") for event in process_events if event.get("drop_count") is not None]
if drop_counts and (len(drop_counts) != len(process_events) or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in drop_counts)):
    raise SystemExit("process drop counts must be complete and non-negative")
print(f"PipeWire transition evidence verified ({len(events)} events)")
