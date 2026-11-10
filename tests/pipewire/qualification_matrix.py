"""Create an auditable cross-platform native qualification status matrix."""
import json
import os
import platform
import stat
import sys
import tempfile
from pathlib import Path

if len(sys.argv) not in (2, 4) or (len(sys.argv) == 4 and sys.argv[2] != "--summary"):
    raise SystemExit("usage: qualification_matrix.py OUTPUT.json [--summary SUMMARY.json]")
output = Path(sys.argv[1])
if output.is_symlink():
    raise SystemExit("qualification matrix output is a symbolic link")
if output.exists() and not output.is_file():
    raise SystemExit("qualification matrix output is not a regular file")
if output.parent.is_symlink():
    raise SystemExit("qualification matrix output directory is a symbolic link")
if len(sys.argv) == 4:
    try:
        summary = json.loads(Path(sys.argv[3]).read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SystemExit(f"malformed native qualification summary: {error}")
else:
    summary = None
summary_path = Path(sys.argv[3]).resolve() if len(sys.argv) == 4 else None
host = platform.system()
platforms = {
    "Linux": {"target": "Linux/PipeWire", "status": "pending", "evidence": [], "execution_blockers": ["Linux host", "PipeWire daemon", "libpipewire-0.3", "audio sink"]},
    "Windows": {"target": "Windows/native audio/overlay", "status": "pending", "evidence": [], "execution_blockers": ["Windows host", "desktop audio permission", "overlay session"]},
    "Darwin": {"target": "macOS/native audio/overlay", "status": "pending", "evidence": [], "execution_blockers": ["macOS host", "Screen Recording permission", "audio capture permission", "overlay session"]},
}
for item in platforms.values():
    if item["status"] == "pending":
        item["pending_reason"] = "native host execution pending"
if summary is not None:
    if not isinstance(summary, dict):
        raise SystemExit("native qualification summary must be an object")
    if "schema" in summary and summary["schema"] != "beatglass-native-qualification-summary-v1":
        raise SystemExit("native qualification summary has unsupported schema")
    if not isinstance(summary.get("native_artifacts", []), list) or any(not isinstance(artifact, str) or not artifact.strip() or any(ord(char) < 32 for char in artifact) for artifact in summary.get("native_artifacts", [])):
        raise SystemExit("native qualification summary has invalid native_artifacts")
    if len(set(summary.get("native_artifacts", []))) != len(summary.get("native_artifacts", [])):
        raise SystemExit("native qualification summary has duplicate native_artifacts")
    if not isinstance(summary.get("qualified"), bool):
        raise SystemExit("native qualification summary has invalid qualified status")
    if "native_execution_attempted" in summary and not isinstance(summary["native_execution_attempted"], bool):
        raise SystemExit("native qualification summary has invalid execution attempt")
    if "native_execution_blockers" in summary and (not isinstance(summary["native_execution_blockers"], list) or
            any(not isinstance(blocker, str) or not blocker.strip() or any(ord(char) < 32 for char in blocker) for blocker in summary["native_execution_blockers"])):
        raise SystemExit("native qualification summary has invalid execution blockers")
    if "native_execution_blockers" in summary and len(set(summary["native_execution_blockers"])) != len(summary["native_execution_blockers"]):
        raise SystemExit("native qualification summary has duplicate execution blockers")
    if summary.get("qualified") is False and (not isinstance(summary.get("pending_reason"), str) or not summary["pending_reason"].strip() or summary["pending_reason"] != summary["pending_reason"].strip() or any(ord(char) < 32 for char in summary["pending_reason"])):
        raise SystemExit("native qualification summary has invalid pending reason")
    if summary.get("qualified") is True and (not isinstance(summary.get("commit"), str) or not summary["commit"].strip() or summary["commit"] != summary["commit"].strip()):
        raise SystemExit("native qualification summary has missing commit provenance")
    if summary.get("qualified") is True and summary.get("native_execution_attempted") is not True:
        raise SystemExit("native qualification summary lacks native execution attempt")
    if summary.get("qualified") is True and summary.get("native_execution_blockers"):
        raise SystemExit("native qualification summary retains execution blockers")
    if "commit" in summary and (not isinstance(summary["commit"], str) or not summary["commit"].strip() or summary["commit"] != summary["commit"].strip() or any(ord(char) < 32 for char in summary["commit"])):
        raise SystemExit("native qualification summary has invalid commit provenance")
    if "host_os" in summary and summary["host_os"] != host:
        raise SystemExit("native qualification summary host does not match runner host")
    current = platforms.get(host)
    if current is not None:
        current["status"] = "qualified" if summary.get("qualified") is True else "pending"
        evidence = []
        for artifact in summary.get("native_artifacts", []):
            artifact_path = Path(artifact)
            if artifact_path.is_absolute() and summary_path is not None:
                try:
                    artifact = artifact_path.resolve().relative_to(summary_path.parent).as_posix()
                except ValueError:
                    artifact = artifact_path.name
            evidence.append(artifact)
        if len(set(evidence)) != len(evidence):
            raise SystemExit("native qualification summary artifacts collide after normalization")
        current["evidence"] = evidence
        current["native_execution_attempted"] = summary.get("native_execution_attempted", False)
        if current["status"] == "pending":
            current["execution_blockers"] = summary.get("native_execution_blockers", [])
        if current["status"] == "pending":
            current["pending_reason"] = summary.get("pending_reason", "native qualification incomplete")
matrix = {"schema": "beatglass-native-qualification-matrix-v1", "host_os": host, "python": sys.version.split()[0], "platforms": platforms}
if summary is not None and isinstance(summary.get("commit"), str) and summary["commit"].strip():
    matrix["commit"] = summary["commit"]
temporary = None
try:
    output.parent.mkdir(parents=True, exist_ok=True)
    existing_mode = stat.S_IMODE(output.stat().st_mode) if output.exists() else None
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=output.parent, delete=False) as handle:
        temporary = Path(handle.name)
        handle.write(json.dumps(matrix, indent=2) + "\n")
        handle.flush()
        os.fsync(handle.fileno())
    if existing_mode is not None:
        os.chmod(temporary, existing_mode)
    os.replace(temporary, output)
    if os.name != "nt":
        directory_fd = os.open(output.parent, os.O_RDONLY)
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
except OSError as error:
    if temporary is not None:
        temporary.unlink(missing_ok=True)
    raise SystemExit(f"qualification matrix output is unwritable: {error}")
print(f"Wrote qualification matrix for {len(platforms)} platforms to {output}")
