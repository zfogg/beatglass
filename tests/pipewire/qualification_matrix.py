"""Create an auditable cross-platform native qualification status matrix."""
import json
import platform
import sys
from pathlib import Path

if len(sys.argv) not in (2, 4) or (len(sys.argv) == 4 and sys.argv[2] != "--summary"):
    raise SystemExit("usage: qualification_matrix.py OUTPUT.json [--summary SUMMARY.json]")
output = Path(sys.argv[1])
summary = json.loads(Path(sys.argv[3]).read_text(encoding="utf-8")) if len(sys.argv) == 4 else None
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
    if not isinstance(summary.get("native_artifacts", []), list) or any(not isinstance(artifact, str) or not artifact.strip() for artifact in summary.get("native_artifacts", [])):
        raise SystemExit("native qualification summary has invalid native_artifacts")
    if len(set(summary.get("native_artifacts", []))) != len(summary.get("native_artifacts", [])):
        raise SystemExit("native qualification summary has duplicate native_artifacts")
    if not isinstance(summary.get("qualified"), bool):
        raise SystemExit("native qualification summary has invalid qualified status")
    if "native_execution_attempted" in summary and not isinstance(summary["native_execution_attempted"], bool):
        raise SystemExit("native qualification summary has invalid execution attempt")
    if "native_execution_blockers" in summary and (not isinstance(summary["native_execution_blockers"], list) or
            any(not isinstance(blocker, str) or not blocker.strip() for blocker in summary["native_execution_blockers"])):
        raise SystemExit("native qualification summary has invalid execution blockers")
    if summary.get("qualified") is False and (not isinstance(summary.get("pending_reason"), str) or not summary["pending_reason"].strip()):
        raise SystemExit("native qualification summary has invalid pending reason")
    if summary.get("qualified") is True and (not isinstance(summary.get("commit"), str) or not summary["commit"].strip()):
        raise SystemExit("native qualification summary has missing commit provenance")
    if summary.get("qualified") is True and summary.get("native_execution_attempted") is not True:
        raise SystemExit("native qualification summary lacks native execution attempt")
    if summary.get("qualified") is True and summary.get("native_execution_blockers"):
        raise SystemExit("native qualification summary retains execution blockers")
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
        current["evidence"] = evidence
        current["native_execution_attempted"] = summary.get("native_execution_attempted", False)
        if current["status"] == "pending":
            current["execution_blockers"] = summary.get("native_execution_blockers", [])
        if current["status"] == "pending":
            current["pending_reason"] = summary.get("pending_reason", "native qualification incomplete")
matrix = {"schema": "beatglass-native-qualification-matrix-v1", "host_os": host, "python": sys.version.split()[0], "platforms": platforms}
if summary is not None and isinstance(summary.get("commit"), str) and summary["commit"].strip():
    matrix["commit"] = summary["commit"]
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
print(f"Wrote qualification matrix for {len(platforms)} platforms to {output}")
