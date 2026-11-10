"""Check that every documented platform row has an attributable evidence bundle."""
import argparse
import hashlib
import importlib.util
import json
import os
import tempfile
from pathlib import Path
import sys
from datetime import datetime, timezone
import re
from collections import Counter
MAX_REPORT_BYTES = 16 << 20

spec = importlib.util.spec_from_file_location("verify_bundle", Path(__file__).with_name("verify_evidence_bundle.py"))
verifier = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(verifier)


def platforms_from_status(path: Path) -> list[str]:
    return [line.split("|")[1].strip().lower() for line in path.read_text(encoding="utf-8").splitlines()
            if line.startswith(("| Windows |", "| macOS |", "| Linux |"))]


def identity_errors(data: dict, max_age_hours: float) -> list[str]:
    errors = []
    commit = data.get("commit")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        errors.append("invalid source commit identity")
    try:
        captured = datetime.fromisoformat(str(data.get("captured_at", "")).replace("Z", "+00:00"))
        age_hours = (datetime.now(timezone.utc) - captured.astimezone(timezone.utc)).total_seconds() / 3600
        if age_hours > max_age_hours:
            errors.append(f"evidence is stale ({age_hours:.1f}h > {max_age_hours:.1f}h)")
        if age_hours < -1:
            errors.append("capture time is in the future")
    except (TypeError, ValueError):
        errors.append("invalid capture timestamp")
    return errors


def check(status: Path, bundles: Path, max_age_hours: float = 168) -> list[str]:
    errors = []
    platforms = platforms_from_status(status)
    if len(platforms) != len(set(platforms)):
        errors.append("status matrix contains duplicate platform rows")
    for platform in platforms:
        bundle = bundles / platform
        manifest = bundle / "manifest.json"
        if not manifest.is_file():
            errors.append(f"{platform}: missing manifest: {manifest}")
            continue
        try:
            data = json.loads(manifest.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{platform}: invalid manifest: {exc}")
            continue
        if not isinstance(data, dict):
            errors.append(f"{platform}: manifest root must be an object")
            continue
        if not isinstance(data.get("artifacts"), list):
            data["artifacts"] = []
        if not isinstance(data.get("scenarios"), list):
            data["scenarios"] = []
        if str(data.get("platform", "")).lower() != platform:
            errors.append(f"{platform}: manifest platform does not match status row")
        scenario_status = {item.get("id"): item.get("status") for item in data.get("scenarios", [])
                           if isinstance(item, dict) and isinstance(item.get("id"), str)}
        unpassed = sorted(scenario_id for scenario_id in verifier.REQUIRED_SCENARIOS if scenario_status.get(scenario_id) != "PASS")
        if unpassed:
            errors.append(f"{platform}: required scenarios are not PASS: {unpassed}")
        errors.extend(f"{platform}: {error}" for error in identity_errors(data, max_age_hours))
        errors.extend(f"{platform}: {error}" for error in verifier.verify(bundle, manifest))
    return errors


def report(status: Path, bundles: Path, max_age_hours: float = 168) -> dict:
    rows = []
    platforms = platforms_from_status(status)
    expected_platforms = ["windows", "macos", "linux"]
    matrix_errors = ["status matrix contains duplicate platform rows"] if len(platforms) != len(set(platforms)) else []
    for platform in platforms:
        bundle = bundles / platform
        manifest = bundle / "manifest.json"
        identity_failures = []
        provenance_complete = False
        missing_provenance = []
        missing_scenarios = []
        invalid_scenarios = []
        manifest_platform = None
        errors = []
        if manifest.is_file():
            try:
                data = json.loads(manifest.read_text(encoding="utf-8"))
                if not isinstance(data, dict):
                    errors = [f"{platform}: manifest root must be an object"]
                    data = {}
                if not isinstance(data.get("artifacts"), list):
                    data["artifacts"] = []
                if not isinstance(data.get("scenarios"), list):
                    data["scenarios"] = []
                manifest_platform = str(data.get("platform", "")).lower() or None
                provenance_complete = all(isinstance(data.get(field), str) and data[field].strip()
                                          for field in verifier.REQUIRED_PROVENANCE)
                missing_provenance = [field for field in verifier.REQUIRED_PROVENANCE
                                      if not isinstance(data.get(field), str) or not data[field].strip()]
                scenario_ids = {item.get("id") for item in data.get("scenarios", [])
                                if isinstance(item, dict) and isinstance(item.get("id"), str)}
                missing_scenarios = sorted(verifier.REQUIRED_SCENARIOS - scenario_ids)
                invalid_scenarios = [item.get("id") if isinstance(item, dict) else str(item)
                                     for item in data.get("scenarios", [])
                                     if (not isinstance(item, dict) or not isinstance(item.get("status"), str)
                                         or item.get("status") not in {"PASS", "FAIL", "UNRUN"})]
                errors = [f"{platform}: {error}" for error in verifier.verify(bundle, manifest)]
                if str(data.get("platform", "")).lower() != platform:
                    errors.append(f"{platform}: manifest platform does not match status row")
                identity_failures = identity_errors(data, max_age_hours)
                errors.extend(f"{platform}: {error}" for error in identity_failures)
                scenario_ids = [item.get("id") for item in data.get("scenarios", [])
                                if isinstance(item, dict) and isinstance(item.get("id"), str)]
                if len(scenario_ids) != len(set(scenario_ids)):
                    errors.append(f"{platform}: duplicate scenario id in manifest")
                retained = len(data.get("artifacts", []))
                declared_paths = {entry.get("path") for entry in data.get("artifacts", [])
                                  if isinstance(entry, dict) and isinstance(entry.get("path"), str)}
                actual_files = {path.relative_to(bundle).as_posix() for path in bundle.rglob("*") if path.is_file()
                                and path.name not in {"readiness-report.json", "manifest.json"}}
                unlisted = sorted(actual_files - declared_paths)
                if unlisted:
                    errors.append(f"{platform}: unlisted retained files: {unlisted}")
                artifact_bytes = sum((bundle / entry["path"]).stat().st_size for entry in data.get("artifacts", [])
                                     if isinstance(entry, dict) and isinstance(entry.get("path"), str)
                                     and (bundle / entry["path"]).is_file())
                manifest_sha256 = hashlib.sha256(manifest.read_bytes()).hexdigest()
                inventory = []
                for entry in sorted(data.get("artifacts", []), key=lambda item: str(item.get("path", ""))):
                    raw_path = entry.get("path", "")
                    path = bundle / raw_path if isinstance(raw_path, str) else None
                    actual_hash = hashlib.sha256(path.read_bytes()).hexdigest() if path and path.is_file() else None
                    inventory.append({"path": entry.get("path"), "sha256": entry.get("sha256"),
                                      "bytes": path.stat().st_size if path and path.is_file() else None,
                                      "hash_valid": actual_hash == str(entry.get("sha256", "")).lower()})
                missing_artifacts = sorted((item["path"] for item in inventory if item["bytes"] is None), key=str)
                mismatched_artifacts = sorted(
                    (item["path"] for item in inventory if not item["hash_valid"] and item["bytes"] is not None),
                    key=str,
                )
                scenario_inventory = sorted(
                    [{"id": item.get("id"), "status": item.get("status"), "evidence": item.get("evidence")}
                     for item in data.get("scenarios", []) if isinstance(item, dict)],
                    key=lambda item: str(item.get("id", "")))
                scenario_status_counts = dict(sorted(Counter(str(item["status"]) for item in scenario_inventory).items()))
                scenario_status_by_id = {item.get("id"): item.get("status") for item in scenario_inventory
                                         if isinstance(item.get("id"), str)}
                unpassed_scenarios = sorted(
                    scenario_id for scenario_id in verifier.REQUIRED_SCENARIOS
                    if scenario_status_by_id.get(scenario_id) != "PASS")
                if unpassed_scenarios:
                    errors.append(f"{platform}: required scenarios are not PASS: {unpassed_scenarios}")
                artifact_roles = {entry.get("path"): entry.get("role") for entry in data.get("artifacts", [])
                                  if isinstance(entry, dict) and isinstance(entry.get("path"), str)}
                missing_artifact_roles = sorted(
                    item.get("id") for item in data.get("scenarios", [])
                    if isinstance(item, dict) and item.get("status") != "UNRUN"
                    and (not isinstance(item.get("evidence"), str)
                         or not artifact_roles.get(item.get("evidence"))))
            except (OSError, json.JSONDecodeError) as exc:
                errors = [f"{platform}: invalid manifest: {exc}"]
                retained = 0
                artifact_bytes = 0
                manifest_sha256 = None
                inventory = []
                scenario_inventory = []
                scenario_status_counts = {}
                unpassed_scenarios = []
                missing_artifacts = []
                mismatched_artifacts = []
                invalid_scenarios = []
                artifact_roles = {}
                missing_artifact_roles = []
        else:
            retained = 0
            artifact_bytes = 0
            manifest_sha256 = None
            inventory = []
            scenario_inventory = []
            scenario_status_counts = {}
            unpassed_scenarios = sorted(verifier.REQUIRED_SCENARIOS)
            missing_artifacts = []
            mismatched_artifacts = []
            invalid_scenarios = []
            artifact_roles = {}
            missing_artifact_roles = []
            missing_scenarios = sorted(verifier.REQUIRED_SCENARIOS)
            missing_provenance = list(verifier.REQUIRED_PROVENANCE)
            errors = [f"{platform}: missing manifest: {manifest}"]
        row_categories = Counter()
        for error in matrix_errors + errors:
            if "hash mismatch" in error: row_categories["hash"] += 1
            elif "scenario" in error or "artifact role" in error: row_categories["scenario"] += 1
            elif "missing artifact" in error or "missing manifest" in error: row_categories["retention"] += 1
            elif "stale" in error or "future" in error: row_categories["freshness"] += 1
            elif "path" in error or "escapes bundle" in error: row_categories["path"] += 1
            else: row_categories["manifest"] += 1
        rows.append({"platform": platform, "manifest": str(manifest), "manifest_sha256": manifest_sha256,
                     "retained_artifacts": retained, "retained_bytes": artifact_bytes,
                     "missing_artifacts": missing_artifacts, "mismatched_artifacts": mismatched_artifacts,
                     "artifact_inventory": inventory, "scenario_inventory": scenario_inventory,
                     "scenario_status_counts": scenario_status_counts,
                     "artifact_roles": artifact_roles, "missing_artifact_roles": missing_artifact_roles,
                     "unpassed_scenarios": unpassed_scenarios,
                     "fresh": manifest_sha256 is not None and not any("stale" in error or "future" in error for error in identity_failures),
                     "freshness_errors": [error for error in identity_failures if "stale" in error or "future" in error or "timestamp" in error],
                     "manifest_platform": manifest_platform, "provenance_complete": provenance_complete,
                     "missing_provenance": missing_provenance,
                     "missing_scenarios": missing_scenarios,
                     "invalid_scenarios": invalid_scenarios,
                     "failure_categories": dict(sorted(row_categories.items())),
                     "failure_category_count_consistent": sum(row_categories.values()) == len(matrix_errors + errors),
                     "error_count": len(matrix_errors + errors),
                     "retention_ok": not missing_artifacts and not mismatched_artifacts,
                     "invalidated": bool(matrix_errors + errors), "errors": matrix_errors + errors})
    platform_summary_consistent = sorted(platforms) == expected_platforms
    platform_failures = [{"platform": row["platform"], "errors": row["error_count"],
                          "fresh": row["fresh"], "retention_ok": row["retention_ok"],
                          "invalidated": row["invalidated"], "ready": row["error_count"] == 0 and row["fresh"] and row["retention_ok"]}
                         for row in rows]
    native_qualification_summary = [
        {"platform": row["platform"],
         "evidence_complete": not row["unpassed_scenarios"] and not row["missing_artifact_roles"],
         "unpassed_scenarios": row["unpassed_scenarios"],
         "missing_artifact_roles": row["missing_artifact_roles"],
         "verifier_errors": row["error_count"],
         "ready": next(item["ready"] for item in platform_failures if item["platform"] == row["platform"])}
        for row in rows]
    native_qualification_blockers = [
        {"platform": row["platform"], "unpassed_scenarios": row["unpassed_scenarios"],
         "missing_artifact_roles": row["missing_artifact_roles"], "errors": row["errors"]}
        for row in rows if row["unpassed_scenarios"] or row["missing_artifact_roles"] or row["errors"]]
    expected_blocker_platforms = {row["platform"] for row in rows if row["unpassed_scenarios"] or row["missing_artifact_roles"] or row["errors"]}
    native_blockers_consistent = {item["platform"] for item in native_qualification_blockers} == expected_blocker_platforms
    platform_support_status = []
    for item in native_qualification_summary:
        blockers = next((entry for entry in native_qualification_blockers if entry["platform"] == item["platform"]), None)
        platform_support_status.append({"platform": item["platform"],
            "status": "READY" if item["ready"] and item["evidence_complete"] else "PENDING",
            "reason_codes": (["scenario"] if item["unpassed_scenarios"] or item["missing_artifact_roles"] else []) + (["verifier"] if item["verifier_errors"] else []),
            "blocker_count": len(blockers["errors"]) if blockers else 0})
    platform_support_status_consistent = (
        [item["platform"] for item in platform_support_status] == [item["platform"] for item in native_qualification_summary]
        and all(item["status"] == ("READY" if summary["ready"] and summary["evidence_complete"] else "PENDING")
                for item, summary in zip(platform_support_status, native_qualification_summary)))
    native_summary_consistent = (
        [item["platform"] for item in native_qualification_summary] == [row["platform"] for row in rows]
        and all(item["evidence_complete"] == (not item["unpassed_scenarios"] and not item["missing_artifact_roles"])
                for item in native_qualification_summary)
        and all(item["ready"] == next(row["error_count"] == 0 and row["fresh"] and row["retention_ok"]
                                      for row in rows if row["platform"] == item["platform"])
                for item in native_qualification_summary)
    )
    ready_platforms = sum(item["ready"] for item in platform_failures)
    retained_artifact_count = sum(row["retained_artifacts"] for row in rows)
    retained_artifact_bytes = sum(row["retained_bytes"] for row in rows)
    retained_artifacts_by_platform = {row["platform"]: row["retained_artifacts"] for row in rows}
    retained_bytes_by_platform = {row["platform"]: row["retained_bytes"] for row in rows}
    retained_bytes_consistent = retained_artifact_bytes == sum(row["retained_bytes"] for row in rows)
    inventory_bytes_consistent = all(row["retained_bytes"] == sum(item["bytes"] or 0 for item in row["artifact_inventory"]) for row in rows)
    inventory_count_consistent = all(row["retained_artifacts"] == len(row["artifact_inventory"]) for row in rows)
    error_count_consistent = sum(item["errors"] for item in platform_failures) == len(errors)
    not_ready_platforms = [item["platform"] for item in platform_failures if not item["ready"]]
    failure_summary_consistent = [item["platform"] for item in platform_failures] == [row["platform"] for row in rows]
    errors = [error for row in rows for error in row["errors"]]
    if not platform_summary_consistent:
        errors.append("status matrix must contain exactly Windows, macOS, and Linux rows")
    categories = Counter()
    for error in errors:
        if "hash mismatch" in error: categories["hash"] += 1
        elif "scenario" in error or "artifact role" in error: categories["scenario"] += 1
        elif "escapes bundle" in error or "path" in error: categories["path"] += 1
        elif "stale" in error or "future" in error: categories["freshness"] += 1
        elif "missing artifact" in error or "missing manifest" in error: categories["retention"] += 1
        else: categories["manifest"] += 1
    failure_category_count_consistent = sum(categories.values()) == len(errors)
    category_platforms = {category: [row["platform"] for row in rows if row["failure_categories"].get(category)]
                          for category in sorted(categories)}
    category_platforms_consistent = set(category_platforms) == set(categories)
    category_platform_lists_consistent = all(platform in category_platforms.get(category, [])
                                             for category in category_platforms
                                             for platform in category_platforms[category])
    category_platform_counts = {category: sum(row["failure_categories"].get(category, 0) for row in rows)
                                for category in sorted(categories)}
    category_counts_consistent = category_platform_counts == dict(categories)
    readiness_ready = (not errors and aggregate_consistent and native_summary_consistent and native_blockers_consistent
                       and error_count_consistent and failure_category_count_consistent and category_platforms_consistent
                       and category_platform_lists_consistent and category_counts_consistent and retained_bytes_consistent
                       and inventory_bytes_consistent and inventory_count_consistent and ready_platforms == len(rows))
    required = [{"platform": row["platform"], "missing": row["missing_scenarios"], "unpassed": row["unpassed_scenarios"], "invalid": row["invalid_scenarios"]} for row in rows]
    artifact_roles = [{"platform": row["platform"], "missing": row["missing_artifact_roles"]} for row in rows]
    provenance = [{"platform": row["platform"], "complete": row["provenance_complete"], "missing": row["missing_provenance"]} for row in rows]
    summary_consistent = (len(required) == len(rows) == len(provenance)
                          and [item["platform"] for item in required] == [row["platform"] for row in rows]
                          and [item["platform"] for item in provenance] == [row["platform"] for row in rows])
    aggregate_consistent = (summary_consistent and platform_summary_consistent and failure_summary_consistent
                            and len(required) == len(platform_failures))
    exit_gate_reason_codes = sorted(categories)
    if not platform_summary_consistent:
        exit_gate_reason_codes.append("platform-matrix")
    for name, valid in (("summary-consistency", summary_consistent), ("native-summary-consistency", native_summary_consistent),
                        ("native-blocker-consistency", native_blockers_consistent), ("aggregate-consistency", aggregate_consistent)):
        if not valid and name not in exit_gate_reason_codes:
            exit_gate_reason_codes.append(name)
    return {"readiness_report_schema": 1, "platforms": rows, "platform_count": len(platforms), "expected_platforms": expected_platforms,
            "platform_summary_consistent": platform_summary_consistent,
            "platform_failure_summary": platform_failures,
            "native_qualification_summary": native_qualification_summary,
            "native_qualification_blockers": native_qualification_blockers,
            "native_blockers_consistent": native_blockers_consistent,
            "platform_support_status": platform_support_status,
            "platform_support_status_consistent": platform_support_status_consistent,
            "support_language": "experimental",
            "native_qualification_required": True,
            "native_qualification_ready": bool(native_qualification_summary) and all(item["ready"] and item["evidence_complete"] for item in native_qualification_summary),
            "native_summary_consistent": native_summary_consistent,
            "failure_summary_consistent": failure_summary_consistent,
            "error_count_consistent": error_count_consistent,
            "ready_platform_count": ready_platforms,
            "retained_artifact_count": retained_artifact_count,
            "retained_artifact_bytes": retained_artifact_bytes,
            "retained_artifacts_by_platform": retained_artifacts_by_platform,
            "retained_bytes_by_platform": retained_bytes_by_platform,
            "retained_bytes_consistent": retained_bytes_consistent,
            "inventory_bytes_consistent": inventory_bytes_consistent,
            "inventory_count_consistent": inventory_count_consistent,
            "not_ready_platforms": not_ready_platforms,
            "all_platforms_ready": platform_summary_consistent and ready_platforms == len(rows),
            "error_count": len(errors), "failure_categories": dict(sorted(categories.items())),
            "failure_category_count_consistent": failure_category_count_consistent,
            "failure_category_platforms": category_platforms,
            "category_platforms_consistent": category_platforms_consistent,
            "category_platform_lists_consistent": category_platform_lists_consistent,
            "category_platform_counts": category_platform_counts,
            "category_counts_consistent": category_counts_consistent,
            "required_scenario_summary": required, "artifact_role_summary": artifact_roles, "provenance_summary": provenance,
            "summary_consistent": summary_consistent, "errors": errors,
            "aggregate_consistent": aggregate_consistent,
            "exit_gate_status": "READY" if readiness_ready else "PENDING",
            "exit_gate_reasons": [] if readiness_ready else errors,
            "exit_gate_reason_codes": [] if readiness_ready else sorted(exit_gate_reason_codes),
            "ready": readiness_ready and platform_support_status_consistent}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--status", type=Path, required=True)
    parser.add_argument("--bundles", type=Path, required=True)
    parser.add_argument("--report", type=Path, help="write a JSON retention/readiness report")
    parser.add_argument("--max-age-hours", type=float, default=168, help="reject evidence older than this (default: 168)")
    parser.add_argument("--max-report-bytes", type=int, default=MAX_REPORT_BYTES)
    parser.add_argument("--json", action="store_true", help="emit the readiness report on stdout")
    args = parser.parse_args()
    if args.max_age_hours <= 0:
        print("max age hours must be positive", file=sys.stderr)
        return 1
    errors = check(args.status, args.bundles, args.max_age_hours)
    if args.json:
        print(json.dumps(report(args.status, args.bundles, args.max_age_hours), sort_keys=True))
    if args.report:
        if args.report.is_symlink():
            print("readiness report output must not be a symlink", file=sys.stderr)
            return 1
        if args.report.parent.is_symlink():
            print("readiness report output parent must not be a symlink", file=sys.stderr)
            return 1
        args.report.parent.mkdir(parents=True, exist_ok=True)
        serialized = json.dumps(report(args.status, args.bundles, args.max_age_hours), indent=2) + "\n"
        if args.max_report_bytes <= 0:
            print("report size limit must be positive", file=sys.stderr); return 1
        if len(serialized.encode("utf-8")) > args.max_report_bytes:
            print(f"readiness report exceeds {args.max_report_bytes} bytes", file=sys.stderr); return 1
        temporary = None
        try:
            with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=args.report.parent,
                                             prefix=f".{args.report.name}.", suffix=".tmp", delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(serialized); stream.flush(); os.fsync(stream.fileno())
            os.replace(temporary, args.report); temporary = None
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"evidence readiness covers {len(platforms_from_status(args.status))} platform rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
