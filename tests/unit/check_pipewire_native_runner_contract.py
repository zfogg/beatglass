from pathlib import Path

source = (Path(__file__).parents[2] / "tests/pipewire/run_linux_native_qualification.py").read_text(encoding="utf-8")
for marker in ("check_linux_native_prerequisites.py", "sys.executable", "daemon_preflight", "rev-parse", "libpipewire-0.3", "pipewire_version", "failed_command", "BEATGLASS_BUILD_PIPEWIRE=ON", "BUILD_TESTING=ON", "--show-only=json-v1", "native_test_discovery", "native_test_count", "discovery_count > 0", "validate_pipewire_manifest.py", "validate_index", "json.loads", "ET.parse", "native_junit_test_count", "^pipewire_tests$", "--no-tests=error", "--output-junit", "native_junit", "native_junit_exists", "is_file", "--timeout", "30", "native_pipewire_tests", "capture_output=True", "capture_index", "duration_seconds", "stdout", "stderr", "ctest", "capture_pipewire_manifest.py", "qualified", "Linux native qualification pending"):
    assert marker in source
print("PipeWire native runner contract verified (5 checks)")
