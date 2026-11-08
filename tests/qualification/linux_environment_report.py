#!/usr/bin/env python3
"""Report Linux qualification prerequisites without claiming support."""
import json
import platform
import shutil
import subprocess
import sys


COMMANDS = ("cmake", "pipewire", "wireplumber", "pw-cli", "wpctl", "dbus-run-session")


def version(command):
    try:
        result = subprocess.run((command, "--version"), capture_output=True, text=True, check=False)
    except OSError:
        return None
    line = (result.stdout or result.stderr).splitlines()
    return line[0] if line else None


def main():
    report = {
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "commands": {name: {"path": shutil.which(name), "version": version(name)} for name in COMMANDS},
        "native_linux_claim": False,
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["system"] == "Linux" else 2


if __name__ == "__main__":
    sys.exit(main())
