"""Run the documented Linux PipeWire configure/build/CTest qualification pass."""
import argparse
import json
import platform
import subprocess
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, default=Path(__file__).parents[2])
parser.add_argument("--build", type=Path, default=Path("build-pipewire"))
parser.add_argument("--summary", type=Path)
args = parser.parse_args()
if platform.system() != "Linux":
    print("Linux native qualification pending: run on Linux")
    raise SystemExit(0)
commit = subprocess.run(["git", "-C", str(args.source), "rev-parse", "HEAD"], check=False, capture_output=True, text=True).stdout.strip()
pipewire_version = subprocess.run(["pkg-config", "--modversion", "libpipewire-0.3"], check=False, capture_output=True, text=True).stdout.strip()
commands = [
    ["python", str(args.source / "tests/pipewire/check_linux_native_prerequisites.py")],
    ["cmake", "-S", str(args.source), "-B", str(args.build), "-DBEATGLASS_BUILD_PIPEWIRE=ON", "-DBUILD_TESTING=ON"],
    ["cmake", "--build", str(args.build)],
    ["ctest", "--test-dir", str(args.build), "--output-on-failure", "-R", "pipewire"],
    ["ctest", "--test-dir", str(args.build), "--show-only=json-v1", "--no-tests=error", "-R", "^pipewire_tests$"],
    ["ctest", "--test-dir", str(args.build), "--output-on-failure", "--no-tests=error", "--timeout", "30", "-R", "^pipewire_tests$"],
]
evidence = args.summary.parent if args.summary else args.source / "evidence"
evidence.mkdir(parents=True, exist_ok=True)
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
    capture = ["python", str(args.source / "tests/pipewire/capture_pipewire_manifest.py"), str(evidence / "pipewire-manifest.json")]
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
        validate = ["python", str(args.source / "tests/pipewire/validate_pipewire_manifest.py"), str(evidence / "pipewire-manifest.json")]
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
if discovery_result:
    try:
        discovery_json = json.loads(Path(discovery_result["stdout"]).read_text(encoding="utf-8"))
        discovery_count = len(discovery_json.get("tests", []))
    except (OSError, json.JSONDecodeError, AttributeError):
        discovery_count = 0
summary = {"platform": platform.platform(), "commit": commit,
           "pipewire_version": pipewire_version, "commands": results,
           "failed_command": next((item["command"] for item in results if item["returncode"]), None),
           "native_test_discovery": discovery_result,
           "native_test_count": discovery_count,
           "daemon_preflight": preflight_result,
           "native_pipewire_tests": native_result,
           "qualified": discovery_count is not None and discovery_count > 0 and all(item["returncode"] == 0 for item in results)}
if args.summary:
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    args.summary.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(json.dumps(summary, indent=2))
raise SystemExit(0 if summary["qualified"] else 1)
