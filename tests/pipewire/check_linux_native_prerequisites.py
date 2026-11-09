"""Check the Linux PipeWire development prerequisites without requiring Linux on CI."""
import platform
import json
import shutil
import subprocess
import sys

if platform.system() != "Linux":
    print("PipeWire native preflight pending: run on Linux with libpipewire-0.3")
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
        if not isinstance(inventory, list):
            missing.append("valid pw-dump JSON")
        elif not any(
            (props := item.get("info", {}).get("props", {})).get("media.class") == "Audio/Sink"
            and str(props.get("object.serial", "")).strip()
            and str(props.get("node.name", "")).strip()
            for item in inventory if isinstance(item, dict)
        ):
            missing.append("PipeWire Audio/Sink node")
        else:
            sink_serials = [
                str(item["info"]["props"]["object.serial"]).strip()
                for item in inventory if isinstance(item, dict)
                and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink"
            ]
            if len(sink_serials) != len(set(sink_serials)):
                missing.append("unique PipeWire output sink serials")
            sink_names = [
                str(item["info"]["props"]["node.name"]).strip()
                for item in inventory if isinstance(item, dict)
                and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink"
            ]
            if len(sink_names) != len(set(sink_names)):
                missing.append("unique PipeWire output sink node names")
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
if missing:
    print("Missing Linux PipeWire prerequisites: " + ", ".join(missing), file=sys.stderr)
    print("Install PipeWire development headers and rerun CMake with BEATGLASS_BUILD_PIPEWIRE=ON", file=sys.stderr)
    raise SystemExit(1)
print("Linux PipeWire prerequisites available; configure and run pipewire_tests")
