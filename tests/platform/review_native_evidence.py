"""Create a conservative reviewer assessment from retained native evidence."""
import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

REQUIRED = {"capture", "recovery", "output", "display", "playback", "overlay"}


def assess(report_path: Path, reviewer: str) -> dict:
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return {"native_review_schema": 1, "decision": "PENDING", "reviewer": reviewer.strip(), "blockers": [str(error)]}
    blockers = []
    seen_platforms = set()
    for platform in report.get("platforms", []):
        name = platform.get("platform", "unknown")
        if name in seen_platforms:
            blockers.append(f"{name}:duplicate platform entry")
        seen_platforms.add(name)
        if platform.get("manifest_platform") != name:
            blockers.append(f"{name}:manifest platform mismatch")
        commit = platform.get("commit")
        if not isinstance(commit, str) or len(commit) != 40 or any(char not in "0123456789abcdefABCDEF" for char in commit):
            blockers.append(f"{name}:invalid build commit identity")
        if platform.get("provenance_complete") is not True:
            blockers.append(f"{name}:incomplete provenance")
        manifest_sha = platform.get("manifest_sha256")
        if not isinstance(manifest_sha, str) or len(manifest_sha) != 64 or any(char not in "0123456789abcdefABCDEF" for char in manifest_sha):
            blockers.append(f"{name}:invalid manifest identity")
        if not isinstance(platform.get("qualification_command"), str) or not platform["qualification_command"].strip():
            blockers.append(f"{name}:missing qualification command")
        if not isinstance(platform.get("qualification_output"), str) or not platform["qualification_output"].strip():
            blockers.append(f"{name}:missing qualification output")
        inventory = platform.get("artifact_inventory")
        if not isinstance(inventory, list) or platform.get("retained_artifacts") != len(inventory):
            blockers.append(f"{name}:artifact count mismatch")
        else:
            for artifact in inventory:
                if (not isinstance(artifact, dict) or not artifact.get("path") or
                        not isinstance(artifact.get("sha256"), str) or len(artifact["sha256"]) != 64 or
                        artifact.get("hash_valid") is not True or not isinstance(artifact.get("bytes"), int) or
                        artifact["bytes"] <= 0):
                    blockers.append(f"{name}:invalid artifact provenance")
                    break
            output = platform.get("qualification_output")
            output_sha = platform.get("qualification_output_sha256")
            matching = next((item for item in inventory if isinstance(item, dict) and item.get("path") == output), None)
            if not matching or matching.get("sha256") != output_sha:
                blockers.append(f"{name}:qualification output linkage mismatch")
        rows = [row for row in platform.get("scenarios", []) if isinstance(row, dict)]
        ids = [row.get("id") for row in rows]
        if len(ids) != len(set(ids)):
            blockers.append(f"{name}:duplicate scenario id")
        scenarios = {row.get("id"): row for row in rows}
        if set(scenarios) != REQUIRED:
            blockers.append(f"{name}:scenario inventory incomplete")
        for scenario in sorted(REQUIRED):
            row = scenarios.get(scenario)
            evidence = row.get("evidence") if isinstance(row, dict) else None
            evidence_path = (report_path.parent / evidence).resolve() if isinstance(evidence, str) and evidence.strip() else None
            evidence_valid = (evidence_path is not None and report_path.parent.resolve() in evidence_path.parents
                              and evidence_path.is_file() and evidence_path.stat().st_size > 0)
            if (not row or row.get("status") != "PASS" or not evidence_valid):
                blockers.append(f"{name}:{scenario}")
    if not reviewer.strip():
        blockers.append("reviewer identity is required")
    return {"native_review_schema": 1, "decision": "ACCEPTED" if not blockers else "PENDING",
            "reviewer": reviewer.strip(), "reviewed_at": datetime.now(timezone.utc).isoformat(),
            "report": str(report_path), "blockers": blockers}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("report", type=Path); parser.add_argument("reviewer"); parser.add_argument("output", type=Path)
    args = parser.parse_args(); result = assess(args.report, args.reviewer)
    args.output.parent.mkdir(parents=True, exist_ok=True); args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"native evidence review: {result['decision']}"); return 0 if result["decision"] == "ACCEPTED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
