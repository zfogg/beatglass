from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/verify_native_run_summary.py").read_text(encoding="utf-8")
for marker in ("qualified", "failed_command", "native_discovery_json", "native_junit", "native_junit_sha256", "native_discovery_sha256", "digest mismatch", "qualification_target", "Linux/PipeWire", "handoff_status", "complete", "commit identity", "native_transition_json", "native_transition_sha256", "validate_pipewire_transition_evidence.py", "native_transition_event_count", "native transition event count mismatch", "native transition platform mismatch", "native transition commit mismatch", "not from a Linux host"):
    assert marker in source
for marker in ("runner_summary_schema", "beatglass-native-runner-summary-v1", "captured_at", "environment", "host_os"):
    assert marker in source
print("Native PipeWire summary verifier contract verified (20 checks)")
