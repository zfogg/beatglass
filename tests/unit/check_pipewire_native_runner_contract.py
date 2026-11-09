from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/run_linux_native_qualification.py").read_text(encoding="utf-8")
for marker in ("check_linux_native_prerequisites.py", "sys.executable", "daemon_preflight", "rev-parse", "libpipewire-0.3", "pipewire_version", "failed_command", "BEATGLASS_BUILD_PIPEWIRE=ON", "BUILD_TESTING=ON", "--show-only=json-v1", "native_test_discovery", "native_discovery_json", "native_discovery_sha256", "native_test_count", "native_test_name_match", "discovery_count > 0", "validate_pipewire_manifest.py", "validate_index", "json.loads", "ET.parse", "native_junit_test_count", "native_junit_sha256", "native_junit_tests", "native_junit_failures", "native_junit_errors", "native_junit_skipped", "junit_skipped == 0", "^pipewire_tests$", "--no-tests=error", "--output-junit", "native_junit", "native_junit_exists", "is_file", "--timeout", "30", "native_pipewire_tests", "capture_output=True", "capture_index", "duration_seconds", "stdout", "stderr", "ctest", "capture_pipewire_manifest.py", "qualified", "Linux native qualification pending", "pending_reason", "qualification_target", "handoff_status", "--transitions", "native_transition_json", "native_transition_sha256", "native_transition_event_count", "validate_pipewire_transition_evidence.py", '"complete" if summary["qualified"]', '\"qualified\": False'):
    assert marker in source
for marker in ("--archive", "package_native_evidence.py", "archive packaging failed"):
    assert marker in source
for marker in ("verify_native_evidence_archive.py", "archive verification failed"):
    assert marker in source
for marker in ("requested_summary", "requested_archive", "native_artifacts", "pending_reason"):
    assert marker in source
for marker in ("host_os", "pending_command", "native_execution_attempted", "native_execution_blockers"):
    assert marker in source
assert '"environment"' in source
for marker in ("qualification_matrix_schema", "qualification_matrix_status", "beatglass-native-qualification-matrix-v1"):
    assert marker in source
for marker in ("--capture-plan", "validate_native_host_capture_plan.py", "capture_plan"):
    assert marker in source
assert '"--capture-plan"' in source
assert "capture_plan_sha256" in source
assert "capture_plan_validated" in source
print("PipeWire native runner contract verified (5 checks)")
