"""Run the documented Linux PipeWire configure/build/CTest qualification pass."""
import argparse
import json
import hashlib
import platform
import sys
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, default=Path(__file__).parents[2])
parser.add_argument("--build", type=Path, default=Path("build-pipewire"))
parser.add_argument("--summary", type=Path)
args = parser.parse_args()
if platform.system() != "Linux":
    pending = {"platform": platform.platform(), "commit": None, "pipewire_version": None,
               "commands": [], "failed_command": None, "qualified": False,
               "pending_reason": "Linux native qualification pending: run on Linux",
               "native_test_discovery": None, "native_test_count": None,
               "native_test_name_match": False, "native_junit_exists": False}
    if args.summary:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_text(json.dumps(pending, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(pending, indent=2))
    raise SystemExit(1)
commit = subprocess.run(["git", "-C", str(args.source), "rev-parse", "HEAD"], check=False, capture_output=True, text=True).stdout.strip()
pipewire_version = subprocess.run(["pkg-config", "--modversion", "libpipewire-0.3"], check=False, capture_output=True, text=True).stdout.strip()
commands = [
    [sys.executable, str(args.source / "tests/pipewire/check_linux_native_prerequisites.py")],
    ["cmake", "-S", str(args.source), "-B", str(args.build), "-DBEATGLASS_BUILD_PIPEWIRE=ON", "-DBUILD_TESTING=ON"],
    ["cmake", "--build", str(args.build)],
    ["ctest", "--test-dir", str(args.build), "--output-on-failure", "-R", "pipewire"],
    ["ctest", "--test-dir", str(args.build), "--show-only=json-v1", "--no-tests=error", "-R", "^pipewire_tests$"],
]
evidence = args.summary.parent if args.summary else args.source / "evidence"
evidence.mkdir(parents=True, exist_ok=True)
commands.append(["ctest", "--test-dir", str(args.build), "--output-on-failure", "--output-junit", str(evidence / "pipewire-tests.xml"), "--no-tests=error", "--timeout", "30", "-R", "^pipewire_tests$"])
results = []
for index, command in enumerate(commands):
    started = time.monotonic()
    result = subprocess.run(command, check=False, text=True, capture_output=True)
    stdout_path = evidence / f"native-{index}-stdout.log"
    stderr_path = evidence / f"native-{index}-stderr.log"
    stdout_path.write_text(result.stdout, encoding="utf-8")
    stderr_path.write_text(result.stderr, encoding="utf-8")
    results.append({"command": command, "returncode": result.returncode,
                    "duration_seconds": round(time.monotonic() - started, 3),
                    "stdout": str(stdout_path), "stderr": str(stderr_path)})
    if result.returncode:
        break
if results and results[-1]["returncode"] == 0 and len(results) == len(commands):
    capture = [sys.executable, str(args.source / "tests/pipewire/capture_pipewire_manifest.py"), str(evidence / "pipewire-manifest.json")]
    capture_started = time.monotonic()
    result = subprocess.run(capture, check=False, text=True, capture_output=True)
    capture_index = len(results)
    stdout_path = evidence / f"native-{capture_index}-stdout.log"
    stderr_path = evidence / f"native-{capture_index}-stderr.log"
    stdout_path.write_text(result.stdout, encoding="utf-8")
    stderr_path.write_text(result.stderr, encoding="utf-8")
    results.append({"command": capture, "returncode": result.returncode,
                    "duration_seconds": round(time.monotonic() - capture_started, 3),
                    "stdout": str(stdout_path), "stderr": str(stderr_path)})
    if result.returncode == 0:
        validate = [sys.executable, str(args.source / "tests/pipewire/validate_pipewire_manifest.py"), str(evidence / "pipewire-manifest.json")]
        validate_started = time.monotonic()
        result = subprocess.run(validate, check=False, text=True, capture_output=True)
        validate_index = len(results)
        validate_stdout = evidence / f"native-{validate_index}-stdout.log"
        validate_stderr = evidence / f"native-{validate_index}-stderr.log"
        validate_stdout.write_text(result.stdout, encoding="utf-8")
        validate_stderr.write_text(result.stderr, encoding="utf-8")
        results.append({"command": validate, "returncode": result.returncode,
                        "duration_seconds": round(time.monotonic() - validate_started, 3),
                        "stdout": str(validate_stdout), "stderr": str(validate_stderr)})
native_result = next((item for item in results if "--timeout" in item["command"] and "^pipewire_tests$" in item["command"]), None)
preflight_result = next((item for item in results if "check_linux_native_prerequisites.py" in item["command"]), None)
discovery_result = next((item for item in results if "--show-only=json-v1" in item["command"]), None)
discovery_count = None
discovery_tests = None
discovery_artifact = evidence / "pipewire-tests-discovery.json"
discovery_sha256 = None
if discovery_result:
    try:
        discovery_text = Path(discovery_result["stdout"]).read_text(encoding="utf-8")
        discovery_json = json.loads(discovery_text)
        discovery_artifact.write_text(json.dumps(discovery_json, indent=2) + "\n", encoding="utf-8")
        discovery_sha256 = hashlib.sha256(discovery_artifact.read_bytes()).hexdigest()
        discovery_tests = [item.get("name", "") for item in discovery_json.get("tests", [])]
        discovery_count = len(discovery_tests)
    except (OSError, json.JSONDecodeError, AttributeError):
        discovery_count = 0
junit_count = None
junit_failures = junit_errors = junit_skipped = None
junit_sha256 = None
junit_tests = None
junit_path = evidence / "pipewire-tests.xml"
if junit_path.is_file():
    junit_sha256 = hashlib.sha256(junit_path.read_bytes()).hexdigest()
    try:
        junit_root = ET.parse(junit_path).getroot()
        cases = list(junit_root.iter("testcase"))
        junit_tests = [case.get("name", "") for case in cases]
        junit_count = len(cases)
        junit_failures = sum(1 for case in cases if case.find("failure") is not None)
        junit_errors = sum(1 for case in cases if case.find("error") is not None)
        junit_skipped = sum(1 for case in cases if case.find("skipped") is not None)
    except ET.ParseError:
        junit_count = 0
summary = {"platform": platform.platform(), "commit": commit,
           "pipewire_version": pipewire_version, "commands": results,
           "failed_command": next((item["command"] for item in results if item["returncode"]), None),
           "native_test_discovery": discovery_result,
           "native_discovery_json": str(discovery_artifact),
           "native_discovery_sha256": discovery_sha256,
           "native_test_count": discovery_count,
           "native_test_name_match": discovery_tests is not None and junit_tests is not None and set(discovery_tests) == set(junit_tests),
           "daemon_preflight": preflight_result,
           "native_pipewire_tests": native_result,
           "native_junit": str(evidence / "pipewire-tests.xml"),
           "native_junit_exists": (evidence / "pipewire-tests.xml").is_file(),
           "native_junit_test_count": junit_count,
           "native_junit_sha256": junit_sha256,
           "native_junit_tests": junit_tests,
           "native_junit_failures": junit_failures,
           "native_junit_errors": junit_errors,
           "native_junit_skipped": junit_skipped,
           "qualified": discovery_count is not None and discovery_count > 0 and junit_count is not None and junit_count > 0 and junit_failures == 0 and junit_errors == 0 and junit_skipped == 0 and discovery_tests is not None and junit_tests is not None and set(discovery_tests) == set(junit_tests) and all(item["returncode"] == 0 for item in results)}
if args.summary:
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    args.summary.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(json.dumps(summary, indent=2))
raise SystemExit(0 if summary["qualified"] else 1)
