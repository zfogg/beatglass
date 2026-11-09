"""Run the documented Linux PipeWire configure/build/CTest qualification pass."""
import argparse
import json
import platform
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, default=Path(__file__).parents[2])
parser.add_argument("--build", type=Path, default=Path("build-pipewire"))
parser.add_argument("--summary", type=Path)
args = parser.parse_args()
if platform.system() != "Linux":
    print("Linux native qualification pending: run on Linux")
    raise SystemExit(0)
commands = [
    ["python", str(args.source / "tests/pipewire/check_linux_native_prerequisites.py")],
    ["cmake", "-S", str(args.source), "-B", str(args.build), "-DBEATGLASS_BUILD_PIPEWIRE=ON", "-DBUILD_TESTING=ON"],
    ["cmake", "--build", str(args.build)],
    ["ctest", "--test-dir", str(args.build), "--output-on-failure", "-R", "pipewire"],
    ["ctest", "--test-dir", str(args.build), "--output-on-failure", "-R", "^pipewire_tests$"],
]
results = []
for command in commands:
    result = subprocess.run(command, check=False, text=True)
    results.append({"command": command, "returncode": result.returncode})
    if result.returncode:
        break
if results and results[-1]["returncode"] == 0 and len(results) == len(commands):
    evidence = args.summary.parent if args.summary else args.source / "evidence"
    capture = ["python", str(args.source / "tests/pipewire/capture_pipewire_manifest.py"), str(evidence / "pipewire-manifest.json")]
    result = subprocess.run(capture, check=False, text=True)
    results.append({"command": capture, "returncode": result.returncode})
native_result = next((item for item in results if "^pipewire_tests$" in item["command"]), None)
summary = {"platform": platform.platform(), "commands": results,
           "native_pipewire_tests": native_result,
           "qualified": len(results) == len(commands) and all(item["returncode"] == 0 for item in results)}
if args.summary:
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    args.summary.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print(json.dumps(summary, indent=2))
raise SystemExit(0 if summary["qualified"] else 1)
