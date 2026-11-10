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
        if not isinstance(host.get("name"), str) or not host["name"].strip():
            raise SystemExit("transition host provenance has no host name")
        if host.get("node_id") is not None and (
                not isinstance(host["node_id"], int)
                or isinstance(host["node_id"], bool)
                or host["node_id"] <= 0):
            raise SystemExit("transition host provenance has an invalid node id")
        if host.get("build_id") is not None and (not isinstance(host["build_id"], str) or not host["build_id"].strip()):
            raise SystemExit("transition host provenance has no build id")
        if host.get("kernel") is not None and (not isinstance(host["kernel"], str) or not host["kernel"].strip()):
            raise SystemExit("transition host provenance has no kernel")
        if host.get("os_version") is not None and (not isinstance(host["os_version"], str) or not host["os_version"].strip()):
            raise SystemExit("transition host provenance has no OS version")
        if host.get("arch") is not None and (not isinstance(host["arch"], str) or not host["arch"].strip()):
            raise SystemExit("transition host provenance has no architecture")
        if host.get("container_id") is not None and (not isinstance(host["container_id"], str) or not host["container_id"].strip()):
            raise SystemExit("transition host provenance has no container id")
        if host.get("timezone") is not None and (not isinstance(host["timezone"], str) or not host["timezone"].strip()):
            raise SystemExit("transition host provenance has no timezone")
        if host.get("distro") is not None and (not isinstance(host["distro"], str) or not host["distro"].strip()):
            raise SystemExit("transition host provenance has no distro")
        if host.get("virtualization") is not None and (not isinstance(host["virtualization"], str) or not host["virtualization"].strip()):
            raise SystemExit("transition host provenance has no virtualization context")
        if host.get("hypervisor") is not None and (not isinstance(host["hypervisor"], str) or not host["hypervisor"].strip()):
            raise SystemExit("transition host provenance has no hypervisor")
        if host.get("secure_boot") is not None and not isinstance(host["secure_boot"], bool):
            raise SystemExit("transition host provenance has an invalid secure boot flag")
        if host.get("tpm_present") is not None and not isinstance(host["tpm_present"], bool):
            raise SystemExit("transition host provenance has an invalid TPM flag")
        if host.get("attestation") is not None and (not isinstance(host["attestation"], str) or not host["attestation"].strip()):
            raise SystemExit("transition host provenance has no attestation")
        if host.get("firmware") is not None and (not isinstance(host["firmware"], str) or not host["firmware"].strip()):
            raise SystemExit("transition host provenance has no firmware")
        if host.get("bios") is not None and (not isinstance(host["bios"], str) or not host["bios"].strip()):
            raise SystemExit("transition host provenance has no BIOS")
        if host.get("board") is not None and (not isinstance(host["board"], str) or not host["board"].strip()):
            raise SystemExit("transition host provenance has no board")
        if host.get("serial_number") is not None and (not isinstance(host["serial_number"], str) or not host["serial_number"].strip()):
            raise SystemExit("transition host provenance has no serial number")
        if host.get("cloud_provider") is not None and (not isinstance(host["cloud_provider"], str) or not host["cloud_provider"].strip()):
            raise SystemExit("transition host provenance has no cloud provider")
        if host.get("region") is not None and (not isinstance(host["region"], str) or not host["region"].strip()):
            raise SystemExit("transition host provenance has no region")
        if host.get("availability_zone") is not None and (not isinstance(host["availability_zone"], str) or not host["availability_zone"].strip()):
            raise SystemExit("transition host provenance has no availability zone")
        if host.get("rack_id") is not None and (not isinstance(host["rack_id"], str) or not host["rack_id"].strip()):
            raise SystemExit("transition host provenance has no rack id")
        if host.get("power_state") is not None and (not isinstance(host["power_state"], str) or not host["power_state"].strip()):
            raise SystemExit("transition host provenance has no power state")
        if host.get("thermal_state") is not None and (not isinstance(host["thermal_state"], str) or not host["thermal_state"].strip()):
            raise SystemExit("transition host provenance has no thermal state")
        if host.get("fan_state") is not None and (not isinstance(host["fan_state"], str) or not host["fan_state"].strip()):
            raise SystemExit("transition host provenance has no fan state")
        if host.get("battery_state") is not None and (not isinstance(host["battery_state"], str) or not host["battery_state"].strip()):
            raise SystemExit("transition host provenance has no battery state")
        if host.get("network_state") is not None and (not isinstance(host["network_state"], str) or not host["network_state"].strip()):
            raise SystemExit("transition host provenance has no network state")
        if host.get("dns_state") is not None and (not isinstance(host["dns_state"], str) or not host["dns_state"].strip()):
            raise SystemExit("transition host provenance has no DNS state")
        if host.get("ntp_state") is not None and (not isinstance(host["ntp_state"], str) or not host["ntp_state"].strip()):
            raise SystemExit("transition host provenance has no NTP state")
        if host.get("clock_source") is not None and (not isinstance(host["clock_source"], str) or not host["clock_source"].strip()):
            raise SystemExit("transition host provenance has no clock source")
        if host.get("clock_accuracy_ns") is not None and (
                not isinstance(host["clock_accuracy_ns"], int)
                or isinstance(host["clock_accuracy_ns"], bool)
                or host["clock_accuracy_ns"] < 0):
            raise SystemExit("transition host provenance has invalid clock accuracy")
        if host.get("clock_drift_ppm") is not None and (
                not isinstance(host["clock_drift_ppm"], (int, float))
                or isinstance(host["clock_drift_ppm"], bool)):
            raise SystemExit("transition host provenance has invalid clock drift")
        if host.get("clock_jitter_ns") is not None and (
                not isinstance(host["clock_jitter_ns"], (int, float))
                or isinstance(host["clock_jitter_ns"], bool)
                or host["clock_jitter_ns"] < 0):
            raise SystemExit("transition host provenance has invalid clock jitter")
        if host.get("clock_leap_state") is not None and (not isinstance(host["clock_leap_state"], str) or not host["clock_leap_state"].strip()):
            raise SystemExit("transition host provenance has no leap state")
        if host.get("clock_sync_interval_ms") is not None and (
                not isinstance(host["clock_sync_interval_ms"], int)
                or isinstance(host["clock_sync_interval_ms"], bool)
                or host["clock_sync_interval_ms"] < 0):
            raise SystemExit("transition host provenance has invalid sync interval")
        if host.get("clock_sync_method") is not None and (not isinstance(host["clock_sync_method"], str) or not host["clock_sync_method"].strip()):
            raise SystemExit("transition host provenance has no sync method")
        if host.get("clock_sync_offset_ns") is not None and (
                not isinstance(host["clock_sync_offset_ns"], (int, float))
                or isinstance(host["clock_sync_offset_ns"], bool)):
            raise SystemExit("transition host provenance has invalid sync offset")
        if host.get("clock_holdover_state") is not None and (not isinstance(host["clock_holdover_state"], str) or not host["clock_holdover_state"].strip()):
            raise SystemExit("transition host provenance has no holdover state")
        if host.get("clock_quality") is not None and (not isinstance(host["clock_quality"], str) or not host["clock_quality"].strip()):
            raise SystemExit("transition host provenance has no clock quality")
        if host.get("clock_stratum") is not None and (
                not isinstance(host["clock_stratum"], int)
                or isinstance(host["clock_stratum"], bool)
                or host["clock_stratum"] <= 0):
            raise SystemExit("transition host provenance has invalid clock stratum")
        if host.get("clock_poll_interval_ms") is not None and (
                not isinstance(host["clock_poll_interval_ms"], int)
                or isinstance(host["clock_poll_interval_ms"], bool)
                or host["clock_poll_interval_ms"] < 0):
            raise SystemExit("transition host provenance has invalid poll interval")
        if host.get("clock_source_count") is not None and (
                not isinstance(host["clock_source_count"], int)
                or isinstance(host["clock_source_count"], bool)
                or host["clock_source_count"] <= 0):
            raise SystemExit("transition host provenance has invalid source count")
        if host.get("clock_source_primary") is not None and not isinstance(host["clock_source_primary"], bool):
            raise SystemExit("transition host provenance has an invalid primary-source flag")
        if host.get("clock_source_id") is not None and (not isinstance(host["clock_source_id"], str) or not host["clock_source_id"].strip()):
            raise SystemExit("transition host provenance has no clock source id")
        if host.get("clock_source_type") is not None and (not isinstance(host["clock_source_type"], str) or not host["clock_source_type"].strip()):
            raise SystemExit("transition host provenance has no clock source type")
        if host.get("clock_source_status") is not None and (not isinstance(host["clock_source_status"], str) or not host["clock_source_status"].strip()):
            raise SystemExit("transition host provenance has no clock source status")
        if host.get("clock_source_failover") is not None and not isinstance(host["clock_source_failover"], bool):
            raise SystemExit("transition host provenance has an invalid failover flag")
        if host.get("clock_source_switch_count") is not None and (
                not isinstance(host["clock_source_switch_count"], int)
                or isinstance(host["clock_source_switch_count"], bool)
                or host["clock_source_switch_count"] < 0):
            raise SystemExit("transition host provenance has invalid switch count")
        if host.get("clock_source_last_switch_ns") is not None and (
                not isinstance(host["clock_source_last_switch_ns"], int)
                or isinstance(host["clock_source_last_switch_ns"], bool)
                or host["clock_source_last_switch_ns"] < 0):
            raise SystemExit("transition host provenance has invalid last switch time")
        if host.get("clock_source_health") is not None and (not isinstance(host["clock_source_health"], str) or not host["clock_source_health"].strip()):
            raise SystemExit("transition host provenance has no clock source health")
        if host.get("clock_source_latency_ns") is not None and (
                not isinstance(host["clock_source_latency_ns"], (int, float))
                or isinstance(host["clock_source_latency_ns"], bool)
                or host["clock_source_latency_ns"] < 0):
            raise SystemExit("transition host provenance has invalid source latency")
        if host.get("clock_source_noise_ns") is not None and (
                not isinstance(host["clock_source_noise_ns"], (int, float))
                or isinstance(host["clock_source_noise_ns"], bool)
                or host["clock_source_noise_ns"] < 0):
            raise SystemExit("transition host provenance has invalid source noise")
        if host.get("clock_source_phase_noise") is not None and (
                not isinstance(host["clock_source_phase_noise"], (int, float))
                or isinstance(host["clock_source_phase_noise"], bool)
                or host["clock_source_phase_noise"] < 0):
            raise SystemExit("transition host provenance has invalid phase noise")
        if host.get("clock_source_frequency_hz") is not None and (
                not isinstance(host["clock_source_frequency_hz"], (int, float))
                or isinstance(host["clock_source_frequency_hz"], bool)
                or host["clock_source_frequency_hz"] <= 0):
            raise SystemExit("transition host provenance has invalid source frequency")
        if host.get("clock_source_phase_offset_ns") is not None and (
                not isinstance(host["clock_source_phase_offset_ns"], (int, float))
                or isinstance(host["clock_source_phase_offset_ns"], bool)):
            raise SystemExit("transition host provenance has invalid phase offset")
        if host.get("clock_source_stability") is not None and (not isinstance(host["clock_source_stability"], str) or not host["clock_source_stability"].strip()):
            raise SystemExit("transition host provenance has no source stability")
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
if any(not isinstance(event.get("event"), str) or not event["event"].strip() for event in events):
    raise SystemExit("transition events must have a non-empty event name")
if isinstance(payload, dict) and payload.get("event_count") is not None:
    if (not isinstance(payload["event_count"], int) or isinstance(payload["event_count"], bool)
            or payload["event_count"] <= 0 or payload["event_count"] != len(events)):
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
if epochs and len(epochs) != len(events):
    raise SystemExit("transition epochs must be complete when supplied")
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
if (not isinstance(start.get("epoch"), int) or isinstance(start.get("epoch"), bool)
        or start["epoch"] <= 0 or not isinstance(restart.get("epoch"), int)
        or isinstance(restart.get("epoch"), bool) or restart["epoch"] <= 0):
    raise SystemExit("lifecycle epochs must be positive integers")
if restart.get("epoch") <= start.get("epoch"):
    raise SystemExit("restart does not advance the capture epoch")
if not isinstance(restart.get("serial"), str) or not restart["serial"].strip():
    raise SystemExit("restart event has no sink serial")
if device_ids and (not isinstance(start.get("device_id"), str) or not isinstance(restart.get("device_id"), str)):
    raise SystemExit("device identity is missing from lifecycle boundary")
if start.get("device_id") is not None and restart.get("device_id") is not None and restart["device_id"] != start["device_id"]:
    raise SystemExit("restart device identity does not match start")
error_events = [event for event in events if event.get("event") == "error"]
error_codes = [event.get("error_code") for event in error_events if event.get("error_code") is not None]
if error_codes and (len(error_codes) != len(error_events)
                    or any(not isinstance(value, int) or isinstance(value, bool) or value == 0 for value in error_codes)):
    raise SystemExit("transition error codes must be complete and non-zero")
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
    if reacquisition.get("sample_position") is not None:
        if (not isinstance(reacquisition["sample_position"], int)
                or isinstance(reacquisition["sample_position"], bool)
                or reacquisition["sample_position"] < 0):
            raise SystemExit("reacquisition evidence has an invalid sample position")
    if reacquisition.get("timestamp_ns") is not None:
        if (not isinstance(reacquisition["timestamp_ns"], int)
                or isinstance(reacquisition["timestamp_ns"], bool)
                or reacquisition["timestamp_ns"] <= 0):
            raise SystemExit("reacquisition evidence has an invalid timestamp")
        if isinstance(restart.get("timestamp_ns"), int) and reacquisition["timestamp_ns"] < restart["timestamp_ns"]:
            raise SystemExit("reacquisition timestamp precedes restart")
    if reacquisition.get("drop_count") is not None:
        if (not isinstance(reacquisition["drop_count"], int)
                or isinstance(reacquisition["drop_count"], bool)
                or reacquisition["drop_count"] < 0):
            raise SystemExit("reacquisition evidence has an invalid drop count")
    if reacquisition.get("error_code") is not None:
        if (not isinstance(reacquisition["error_code"], int)
                or isinstance(reacquisition["error_code"], bool)
                or reacquisition["error_code"] == 0):
            raise SystemExit("reacquisition evidence has an invalid error code")
    if reacquisition.get("error_domain") is not None:
        if not isinstance(reacquisition["error_domain"], str) or not reacquisition["error_domain"].strip():
            raise SystemExit("reacquisition evidence has an empty error domain")
    if reacquisition.get("recovery_reason") is not None:
        if not isinstance(reacquisition["recovery_reason"], str) or not reacquisition["recovery_reason"].strip():
            raise SystemExit("reacquisition evidence has an empty recovery reason")
    if reacquisition.get("process_index") is not None:
        if (not isinstance(reacquisition["process_index"], int)
                or isinstance(reacquisition["process_index"], bool)
                or reacquisition["process_index"] < 0):
            raise SystemExit("reacquisition evidence has an invalid process index")
    if reacquisition.get("event_count") is not None:
        if (not isinstance(reacquisition["event_count"], int)
                or isinstance(reacquisition["event_count"], bool)
                or reacquisition["event_count"] <= 0):
            raise SystemExit("reacquisition evidence has an invalid event count")
        restart_process_count = sum(event.get("event") == "process" and event.get("epoch") == restart.get("epoch") for event in events)
        if reacquisition["event_count"] != restart_process_count:
            raise SystemExit("reacquisition event count does not match restart samples")
    if reacquisition.get("artifact_id") is not None:
        if not isinstance(reacquisition["artifact_id"], str) or not reacquisition["artifact_id"].strip():
            raise SystemExit("reacquisition evidence has an empty artifact identity")
        if isinstance(payload, dict) and payload.get("artifact_id") is not None and reacquisition["artifact_id"] != payload["artifact_id"]:
            raise SystemExit("reacquisition artifact identity does not match artifact")
    if reacquisition.get("artifact_sha256") is not None:
        if not isinstance(reacquisition["artifact_sha256"], str) or not re.fullmatch(r"[0-9a-fA-F]{64}", reacquisition["artifact_sha256"]):
            raise SystemExit("reacquisition evidence has an invalid artifact digest")
        if isinstance(payload, dict) and payload.get("artifact_sha256") is not None and reacquisition["artifact_sha256"] != payload["artifact_sha256"]:
            raise SystemExit("reacquisition artifact digest does not match artifact")
    if reacquisition.get("daemon_version") is not None:
        if not isinstance(reacquisition["daemon_version"], str) or not reacquisition["daemon_version"].strip():
            raise SystemExit("reacquisition evidence has an empty daemon version")
    if reacquisition.get("capture_command") is not None:
        if not isinstance(reacquisition["capture_command"], str) or not reacquisition["capture_command"].strip():
            raise SystemExit("reacquisition evidence has an empty capture command")
    if reacquisition.get("capture_started_at") is not None:
        if not isinstance(reacquisition["capture_started_at"], str):
            raise SystemExit("reacquisition evidence has an invalid start timestamp")
        try:
            reacquisition_started_at = datetime.fromisoformat(reacquisition["capture_started_at"].replace("Z", "+00:00"))
        except ValueError:
            raise SystemExit("reacquisition evidence has an invalid start timestamp")
        if reacquisition_started_at.tzinfo is None:
            raise SystemExit("reacquisition start timestamp has no timezone")
    if reacquisition.get("capture_ended_at") is not None:
        if not isinstance(reacquisition["capture_ended_at"], str):
            raise SystemExit("reacquisition evidence has an invalid end timestamp")
        try:
            reacquisition_ended_at = datetime.fromisoformat(reacquisition["capture_ended_at"].replace("Z", "+00:00"))
        except ValueError:
            raise SystemExit("reacquisition evidence has an invalid end timestamp")
        if reacquisition_ended_at.tzinfo is None:
            raise SystemExit("reacquisition end timestamp has no timezone")
        if reacquisition.get("capture_started_at") is not None and reacquisition_ended_at < reacquisition_started_at:
            raise SystemExit("reacquisition end precedes start")
    if reacquisition.get("platform") is not None:
        if reacquisition["platform"] not in ("linux", "windows", "macos"):
            raise SystemExit("reacquisition evidence has an invalid platform")
    if reacquisition.get("node_id") is not None:
        if (not isinstance(reacquisition["node_id"], int)
                or isinstance(reacquisition["node_id"], bool)
                or reacquisition["node_id"] <= 0):
            raise SystemExit("reacquisition evidence has an invalid node id")
    if reacquisition.get("source_name") is not None:
        if not isinstance(reacquisition["source_name"], str) or not reacquisition["source_name"].strip():
            raise SystemExit("reacquisition evidence has an empty source name")
    if reacquisition.get("commit") is not None:
        if not isinstance(reacquisition["commit"], str) or not reacquisition["commit"].strip():
            raise SystemExit("reacquisition evidence has an empty commit")
        if isinstance(payload, dict) and payload.get("commit") and reacquisition["commit"] != payload["commit"]:
            raise SystemExit("reacquisition commit does not match artifact")
    if reacquisition.get("toolchain") is not None:
        if not isinstance(reacquisition["toolchain"], str) or not reacquisition["toolchain"].strip():
            raise SystemExit("reacquisition evidence has an empty toolchain")
    if reacquisition.get("environment") is not None:
        if not isinstance(reacquisition["environment"], str) or not reacquisition["environment"].strip():
            raise SystemExit("reacquisition evidence has an empty environment")
    if reacquisition.get("host") is not None:
        if not isinstance(reacquisition["host"], str) or not reacquisition["host"].strip():
            raise SystemExit("reacquisition evidence has an empty host")
    if reacquisition.get("capture_id") is not None:
        if not isinstance(reacquisition["capture_id"], str) or not reacquisition["capture_id"].strip():
            raise SystemExit("reacquisition evidence has an empty capture id")
    if reacquisition.get("source_id") is not None:
        if not isinstance(reacquisition["source_id"], str) or not reacquisition["source_id"].strip():
            raise SystemExit("reacquisition evidence has an empty source id")
    if reacquisition.get("sink_name") is not None:
        if not isinstance(reacquisition["sink_name"], str) or not reacquisition["sink_name"].strip():
            raise SystemExit("reacquisition evidence has an empty sink name")
    if reacquisition.get("source_type") is not None:
        if not isinstance(reacquisition["source_type"], str) or not reacquisition["source_type"].strip():
            raise SystemExit("reacquisition evidence has an empty source type")
    if reacquisition.get("profile") is not None:
        if not isinstance(reacquisition["profile"], str) or not reacquisition["profile"].strip():
            raise SystemExit("reacquisition evidence has an empty profile")
    if reacquisition.get("policy") is not None:
        if not isinstance(reacquisition["policy"], str) or not reacquisition["policy"].strip():
            raise SystemExit("reacquisition evidence has an empty policy")
    if reacquisition.get("reason_code") is not None:
        if (not isinstance(reacquisition["reason_code"], int)
                or isinstance(reacquisition["reason_code"], bool)
                or reacquisition["reason_code"] == 0):
            raise SystemExit("reacquisition evidence has an invalid reason code")
    if not isinstance(reacquisition.get("serial"), str) or not reacquisition["serial"].strip():
        raise SystemExit("reacquisition evidence has no sink serial")
    if reacquisition["serial"] != restart.get("serial"):
        raise SystemExit("reacquisition sink serial does not match restart")
    if reacquisition.get("device_id") is not None:
        if not isinstance(reacquisition["device_id"], str) or not reacquisition["device_id"].strip():
            raise SystemExit("reacquisition evidence has an empty device identity")
        if reacquisition["device_id"] != restart.get("device_id"):
            raise SystemExit("reacquisition device identity does not match restart")
    reacquisition_format = reacquisition.get("format")
    if reacquisition_format is not None:
        if (not isinstance(reacquisition_format, dict)
                or not isinstance(reacquisition_format.get("sample_rate"), int)
                or isinstance(reacquisition_format.get("sample_rate"), bool)
                or reacquisition_format["sample_rate"] <= 0
                or not isinstance(reacquisition_format.get("channels"), int)
                or isinstance(reacquisition_format.get("channels"), bool)
                or reacquisition_format["channels"] <= 0):
            raise SystemExit("reacquisition evidence has an invalid negotiated format")
        if reacquisition_format != restart.get("format"):
            raise SystemExit("reacquisition format disagrees with restart format")
    restart_index = events.index(restart)
    if not any(event.get("event") == "process" and event.get("epoch") == restart.get("epoch")
               and isinstance(event.get("sample_count"), int) and not isinstance(event.get("sample_count"), bool)
               and event["sample_count"] > 0 for event in events[restart_index + 1:]):
        raise SystemExit("reacquisition evidence has no post-restart samples")
fmt = restart.get("format")
if (not isinstance(fmt, dict)
        or not isinstance(fmt.get("sample_rate"), int)
        or isinstance(fmt.get("sample_rate"), bool)
        or fmt["sample_rate"] <= 0
        or not isinstance(fmt.get("channels"), int)
        or isinstance(fmt.get("channels"), bool)
        or fmt["channels"] <= 0):
    raise SystemExit("restart event has no negotiated format")
for event in events:
    event_format = event.get("format")
    if event_format is not None and (not isinstance(event_format, dict)
            or not isinstance(event_format.get("sample_rate"), int)
            or isinstance(event_format.get("sample_rate"), bool)
            or event_format["sample_rate"] <= 0
            or not isinstance(event_format.get("channels"), int)
            or isinstance(event_format.get("channels"), bool)
            or event_format["channels"] <= 0):
        raise SystemExit("transition event has an invalid negotiated format")
    if event_format is not None and event_format != fmt and event.get("event") == "process":
        raise SystemExit("process format disagrees with restart format")
process_positions = [(event.get("sample_position"), event.get("sample_count")) for event in events if event.get("event") == "process" and event.get("sample_position") is not None]
for previous, current in zip(process_positions, process_positions[1:]):
    if previous[1] is not None and current[0] < previous[0] + previous[1]:
        raise SystemExit("process sample positions overlap captured samples")
process_events = [event for event in events if event.get("event") == "process"]
for event in process_events:
    if (not isinstance(event.get("epoch"), int)
            or isinstance(event.get("epoch"), bool)
            or event["epoch"] <= 0
            or event["epoch"] not in (start["epoch"], restart["epoch"])):
        raise SystemExit("process epoch must match a lifecycle epoch")
process_timestamps = [event["timestamp_ns"] for event in process_events if event.get("timestamp_ns") is not None]
if process_timestamps and any(previous > current for previous, current in zip(process_timestamps, process_timestamps[1:])):
    raise SystemExit("process timestamps must be monotonic")
for previous, current in zip(process_events, process_events[1:]):
    if (previous.get("timestamp_ns") is not None and previous.get("duration_ns") is not None
            and current.get("timestamp_ns") is not None
            and previous["timestamp_ns"] + previous["duration_ns"] > current["timestamp_ns"]):
        raise SystemExit("process intervals overlap")
for event in process_events:
    if event.get("sample_rate") is not None and (
            not isinstance(event["sample_rate"], int)
            or isinstance(event["sample_rate"], bool)
            or event["sample_rate"] <= 0):
        raise SystemExit("process sample rates must be positive integers")
    if event.get("channels") is not None and (
            not isinstance(event["channels"], int)
            or isinstance(event["channels"], bool)
            or event["channels"] <= 0):
        raise SystemExit("process channel counts must be positive integers")
    if event.get("sample_position") is not None and (
            not isinstance(event["sample_position"], int)
            or isinstance(event["sample_position"], bool)
            or event["sample_position"] < 0):
        raise SystemExit("process sample positions must be non-negative integers")
    if event.get("timestamp_ns") is not None and (
            not isinstance(event["timestamp_ns"], int)
            or isinstance(event["timestamp_ns"], bool)
            or event["timestamp_ns"] < 0):
        raise SystemExit("process timestamps must be non-negative integers")
    if event.get("duration_ns") is not None and (
            not isinstance(event["duration_ns"], int)
            or isinstance(event["duration_ns"], bool)
            or event["duration_ns"] < 0):
        raise SystemExit("process durations must be non-negative integers")
sample_counts = [event.get("sample_count") for event in process_events if event.get("sample_count") is not None]
if sample_counts and (len(sample_counts) != len(process_events)
                      or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in sample_counts)):
    raise SystemExit("process sample counts must be complete and non-negative")
process_serials = [event.get("serial") for event in process_events if event.get("serial") is not None]
if process_serials and (len(process_serials) != len(process_events)
                        or any(not isinstance(value, str) or not value.strip() for value in process_serials)):
    raise SystemExit("process sink serials must be complete and non-empty")
if process_serials:
    for event in process_events:
        expected_serial = restart.get("serial") if event.get("epoch") == restart.get("epoch") else start.get("serial")
        if event.get("serial") != expected_serial:
            raise SystemExit("process sink serial does not match active epoch")
drop_counts = [event.get("drop_count") for event in process_events if event.get("drop_count") is not None]
if drop_counts and (len(drop_counts) != len(process_events) or any(not isinstance(value, int) or isinstance(value, bool) or value < 0 for value in drop_counts)):
    raise SystemExit("process drop counts must be complete and non-negative")
print(f"PipeWire transition evidence verified ({len(events)} events)")
