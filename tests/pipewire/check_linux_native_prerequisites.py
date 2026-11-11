"""Check the Linux PipeWire development prerequisites without requiring Linux on CI."""
import platform
import json
import shutil
import subprocess
import sys
import argparse
from pathlib import Path
sys.path.insert(0, str(__import__("pathlib").Path(__file__).parent))
from preflight_inventory import validate_sink_inventory

parser = argparse.ArgumentParser(description="Probe Linux PipeWire prerequisites without claiming capture qualification.")
parser.add_argument("--report", type=Path, help="write a machine-readable probe report")
args = parser.parse_args()

def finish(status, missing, versions=None):
    report = {"platform": platform.platform(), "host_os": platform.system(),
              "target": "Linux/PipeWire", "status": status,
              "native_execution_attempted": False, "missing": missing,
              "versions": versions or {},
              "next_commands": [
                  "python tests/pipewire/check_linux_native_prerequisites.py --report evidence/linux-pipewire-preflight.json",
                  "cmake -S . -B build-pipewire -DBEATGLASS_BUILD_PIPEWIRE=ON -DBUILD_TESTING=ON",
                  "python tests/pipewire/run_linux_native_qualification.py --source . --summary evidence/linux-summary.json",
              ]}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report

if platform.system() != "Linux":
    print(json.dumps(finish("pending", ["Linux host", "PipeWire daemon", "libpipewire-0.3"]), indent=2))
    raise SystemExit(0)

missing = []
if not shutil.which("pkg-config"):
    missing.append("pkg-config")
for tool in ("pw-cli", "pw-dump"):
    if not shutil.which(tool):
        missing.append(tool)
if not missing and subprocess.run(["pw-cli", "info", "0"], check=False,
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
    missing.append("running PipeWire daemon")
if not missing:
    dump = subprocess.run(["pw-dump"], check=False, stdout=subprocess.PIPE, text=True)
    try:
        inventory = json.loads(dump.stdout)
        failure = validate_sink_inventory(inventory)
        if failure:
            missing.append(failure)
    except json.JSONDecodeError:
        missing.append("valid pw-dump JSON")
else:
    result = subprocess.run(["pkg-config", "--exists", "libpipewire-0.3"], check=False)
    if result.returncode:
        missing.append("libpipewire-0.3 development package")
    else:
        version = subprocess.run(["pkg-config", "--modversion", "libpipewire-0.3"], check=False,
                                 stdout=subprocess.PIPE, text=True).stdout.strip()
        if not version:
            missing.append("libpipewire-0.3 version metadata")
        pcfiledir = subprocess.run(["pkg-config", "--variable=pcfiledir", "libpipewire-0.3"],
                                   check=False, stdout=subprocess.PIPE, text=True).stdout.strip()
        if not pcfiledir:
            missing.append("libpipewire-0.3 pkg-config directory")
        for flag in ("--cflags", "--libs"):
            if subprocess.run(["pkg-config", flag, "libpipewire-0.3"], check=False,
                              stdout=subprocess.PIPE, text=True).returncode:
                missing.append(f"pkg-config {flag} for libpipewire-0.3")
# A running daemon does not imply that the development headers and linker
# metadata are installed. Always probe the build capability independently.
if not any(item.startswith("libpipewire-0.3") or item.startswith("pkg-config") for item in missing):
    if not shutil.which("pkg-config") or subprocess.run(["pkg-config", "--exists", "libpipewire-0.3"], check=False).returncode:
        missing.append("libpipewire-0.3 development package")
    else:
        for flag in ("--cflags", "--libs"):
            if subprocess.run(["pkg-config", flag, "libpipewire-0.3"], check=False,
                              stdout=subprocess.PIPE, text=True).returncode:
                missing.append(f"pkg-config {flag} for libpipewire-0.3")
if missing:
    finish("missing", missing)
    print("Missing Linux PipeWire prerequisites: " + ", ".join(missing), file=sys.stderr)
    if any(item in {"running PipeWire daemon", "valid pw-dump JSON"} for item in missing):
        print("Start a logged-in PipeWire desktop session with an output sink, then rerun the native qualification", file=sys.stderr)
    if any(item.startswith("libpipewire-0.3") or item.startswith("pkg-config") for item in missing):
        print("Install PipeWire development headers/pkg-config metadata and rerun CMake with BEATGLASS_BUILD_PIPEWIRE=ON", file=sys.stderr)
    raise SystemExit(1)
version = subprocess.run(["pkg-config", "--modversion", "libpipewire-0.3"], check=False,
                         stdout=subprocess.PIPE, text=True).stdout.strip()
tool_versions = []
for tool in ("pw-cli", "pw-dump"):
    output = subprocess.run([tool, "--version"], check=False, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True).stdout.strip().splitlines()
    tool_versions.append(f"{tool}={output[0] if output else 'unknown'}")
report = finish("ready", [], {"libpipewire-0.3": version, **dict(item.split("=", 1) for item in tool_versions)})
print(json.dumps(report, indent=2))
