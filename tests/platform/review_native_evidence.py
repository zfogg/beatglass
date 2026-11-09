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
    if "ready" in report and not isinstance(report.get("ready"), bool):
        blockers.append("report:ready must be boolean")
    if "native_qualification_required" in report and not isinstance(report.get("native_qualification_required"), bool):
        blockers.append("report:native qualification requirement must be boolean")
    if report.get("readiness_report_schema") not in (None, 1):
        blockers.append("report:unsupported readiness schema")
    if report.get("native_qualification_required") is False and report.get("ready") is True:
        blockers.append("report:ready without native qualification requirement")
    report_platforms = report.get("platforms")
    if not isinstance(report_platforms, list) or not report_platforms:
        blockers.append("report:platform inventory missing")
    elif {item.get("platform") for item in report_platforms if isinstance(item, dict)} != {"linux", "windows", "macos"}:
        blockers.append("report:platform inventory incomplete")
    support = report.get("platform_support_status")
    if isinstance(report_platforms, list) and isinstance(support, list):
        report_names = {item.get("platform") for item in report_platforms if isinstance(item, dict)}
        support_names = {item.get("platform") for item in support if isinstance(item, dict)}
        if report_names != support_names:
            blockers.append("report:platform identity mismatch")
        if len(support_names) != len(support):
            blockers.append("report:duplicate platform support status")
        for item in support:
            if item.get("status") not in {"experimental", "qualified", "unsupported", "pending"}:
                blockers.append("report:invalid platform support status")
    if report.get("exit_gate_status") not in {None, "PENDING", "ACCEPTED"}:
        blockers.append("report:invalid exit gate status")
    if report.get("ready") is True and report.get("exit_gate_status") != "ACCEPTED":
        blockers.append("report:ready status contradicts exit gate")
    if report.get("exit_gate_status") == "ACCEPTED" and report.get("ready") is not True:
        blockers.append("report:accepted gate without ready status")
    reasons = report.get("exit_gate_reason_codes")
    if reasons is not None and (not isinstance(reasons, list) or any(not isinstance(reason, str) or not reason.strip() for reason in reasons)):
        blockers.append("report:invalid exit gate reason codes")
    if isinstance(reasons, list) and len(reasons) != len(set(reasons)):
        blockers.append("report:duplicate exit gate reason codes")
    if report.get("ready") is True and isinstance(reasons, list) and reasons:
        blockers.append("report:ready report has exit gate blockers")
    if report.get("ready") is False and reasons is not None and (not isinstance(reasons, list) or not reasons):
        blockers.append("report:pending report lacks exit gate reason codes")
    handoff_status = report.get("handoff_status")
    if handoff_status not in (None, "pending", "failed", "complete"):
        blockers.append("report:invalid handoff status")
    if report.get("ready") is True and handoff_status not in (None, "complete"):
        blockers.append("report:ready status contradicts handoff status")
    handoff_errors = report.get("handoff_check_errors")
    if report.get("ready") is True and isinstance(handoff_errors, list) and handoff_errors:
        blockers.append("report:ready report has handoff errors")
    documentation_errors = report.get("documentation_check_errors")
    if report.get("ready") is True and isinstance(documentation_errors, list) and documentation_errors:
        blockers.append("report:ready report has documentation errors")
    seen_platforms = set()
    report_commit = report.get("commit")
    for platform in report.get("platforms", []):
        name = platform.get("platform", "unknown")
        if name not in {"linux", "windows", "macos"}:
            blockers.append(f"{name}:unknown platform identity")
        if name in seen_platforms:
            blockers.append(f"{name}:duplicate platform entry")
        seen_platforms.add(name)
        if platform.get("manifest_platform") != name:
            blockers.append(f"{name}:manifest platform mismatch")
        commit = platform.get("commit")
        if not isinstance(commit, str) or len(commit) != 40 or any(char not in "0123456789abcdefABCDEF" for char in commit):
            blockers.append(f"{name}:invalid build commit identity")
        if isinstance(report_commit, str) and commit != report_commit:
            blockers.append(f"{name}:build commit differs from report")
        if platform.get("provenance_complete") is not True:
            blockers.append(f"{name}:incomplete provenance")
        if not isinstance(platform.get("environment"), str) or not platform["environment"].strip():
            blockers.append(f"{name}:missing environment identity")
        captured_at = platform.get("captured_at")
        try:
            parsed_capture = datetime.fromisoformat(captured_at.replace("Z", "+00:00"))
            valid_capture_time = parsed_capture.tzinfo is not None
        except (AttributeError, TypeError, ValueError):
            valid_capture_time = False
        if not valid_capture_time:
            blockers.append(f"{name}:missing capture timestamp")
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
            artifact_paths = [item.get("path") for item in inventory if isinstance(item, dict)]
            if len(artifact_paths) != len(set(artifact_paths)):
                blockers.append(f"{name}:duplicate artifact path")
            for artifact in inventory:
                artifact_path = artifact.get("path") if isinstance(artifact, dict) else None
                if (not isinstance(artifact_path, str) or artifact_path.startswith(("/", "\\")) or
                        ":" in artifact_path or ".." in artifact_path.replace("\\", "/").split("/")):
                    blockers.append(f"{name}:invalid artifact path")
                    break
                if (not isinstance(artifact, dict) or not artifact.get("path") or
                        not isinstance(artifact.get("role"), str) or not artifact["role"].strip() or
                        not isinstance(artifact.get("sha256"), str) or len(artifact["sha256"]) != 64 or
                        any(char not in "0123456789abcdefABCDEF" for char in artifact["sha256"]) or
                        artifact.get("hash_valid") is not True or not isinstance(artifact.get("bytes"), int) or
                        artifact["bytes"] <= 0):
                    blockers.append(f"{name}:invalid artifact provenance")
                    break
            if isinstance(platform.get("retained_bytes"), int) and platform["retained_bytes"] != sum(
                    item.get("bytes", 0) for item in inventory if isinstance(item, dict)):
                blockers.append(f"{name}:retained byte count mismatch")
            output = platform.get("qualification_output")
            output_sha = platform.get("qualification_output_sha256")
            matching = next((item for item in inventory if isinstance(item, dict) and item.get("path") == output), None)
            if not matching or matching.get("sha256") != output_sha:
                blockers.append(f"{name}:qualification output linkage mismatch")
        scenario_rows = platform.get("scenarios")
        inventory_rows = platform.get("scenario_inventory")
        if scenario_rows is not None and inventory_rows is not None and scenario_rows != inventory_rows:
            blockers.append(f"{name}:scenario inventory mismatch")
        if scenario_rows is None:
            scenario_rows = inventory_rows
        rows = [row for row in (scenario_rows or []) if isinstance(row, dict)]
        ids = [row.get("id") for row in rows]
        if len(ids) != len(set(ids)):
            blockers.append(f"{name}:duplicate scenario id")
        scenarios = {row.get("id"): row for row in rows}
        if set(scenarios) != REQUIRED:
            blockers.append(f"{name}:scenario inventory incomplete")
        support_status = next((item.get("status") for item in support or []
                               if isinstance(item, dict) and item.get("platform") == name), None)
        if support_status == "qualified" and any(row.get("status") != "PASS" for row in rows):
            blockers.append(f"{name}:qualified status without all scenarios passing")
        if support_status != "qualified" and rows and all(row.get("status") == "PASS" for row in rows):
            blockers.append(f"{name}:all scenarios pass but platform is not qualified")
        evidence_hashes = platform.get("scenario_evidence_sha256", {})
        if not isinstance(evidence_hashes, dict):
            blockers.append(f"{name}:invalid scenario evidence hashes")
        else:
            for scenario_id, digest in evidence_hashes.items():
                row = scenarios.get(scenario_id)
                matching = next((item for item in inventory if isinstance(item, dict) and item.get("sha256") == digest), None)
                if (row is None or row.get("status") != "PASS" or not isinstance(digest, str) or len(digest) != 64
                        or matching is None):
                    blockers.append(f"{name}:{scenario_id}:invalid scenario evidence hash")
        for scenario in sorted(REQUIRED):
            row = scenarios.get(scenario)
            evidence = row.get("evidence") if isinstance(row, dict) else None
            if row and row.get("status") not in {"PASS", "UNRUN", "FAIL", "BLOCKED"}:
                blockers.append(f"{name}:{scenario}:invalid scenario status")
            if row and row.get("status") != "PASS" and evidence not in (None, ""):
                blockers.append(f"{name}:{scenario}:unexpected evidence for non-pass status")
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
