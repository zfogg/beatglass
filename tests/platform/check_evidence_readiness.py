"""Check that every documented platform row has an attributable evidence bundle."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from datetime import datetime, timezone
import re
from collections import Counter

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
        if str(data.get("platform", "")).lower() != platform:
            errors.append(f"{platform}: manifest platform does not match status row")
        errors.extend(f"{platform}: {error}" for error in identity_errors(data, max_age_hours))
        errors.extend(f"{platform}: {error}" for error in verifier.verify(bundle, manifest))
    return errors


def report(status: Path, bundles: Path, max_age_hours: float = 168) -> dict:
    rows = []
    platforms = platforms_from_status(status)
    matrix_errors = ["status matrix contains duplicate platform rows"] if len(platforms) != len(set(platforms)) else []
    for platform in platforms:
        bundle = bundles / platform
        manifest = bundle / "manifest.json"
        identity_failures = []
        provenance_complete = False
        missing_provenance = []
        missing_scenarios = []
        manifest_platform = None
        errors = []
        if manifest.is_file():
            try:
                data = json.loads(manifest.read_text(encoding="utf-8"))
                manifest_platform = str(data.get("platform", "")).lower() or None
                provenance_complete = all(isinstance(data.get(field), str) and data[field].strip()
                                          for field in verifier.REQUIRED_PROVENANCE)
                missing_provenance = [field for field in verifier.REQUIRED_PROVENANCE
                                      if not isinstance(data.get(field), str) or not data[field].strip()]
                scenario_ids = {item.get("id") for item in data.get("scenarios", []) if isinstance(item, dict)}
                missing_scenarios = sorted(verifier.REQUIRED_SCENARIOS - scenario_ids)
                errors = [f"{platform}: {error}" for error in verifier.verify(bundle, manifest)]
                if str(data.get("platform", "")).lower() != platform:
                    errors.append(f"{platform}: manifest platform does not match status row")
                identity_failures = identity_errors(data, max_age_hours)
                errors.extend(f"{platform}: {error}" for error in identity_failures)
                scenario_ids = [item.get("id") for item in data.get("scenarios", []) if isinstance(item, dict)]
                if len(scenario_ids) != len(set(scenario_ids)):
                    errors.append(f"{platform}: duplicate scenario id in manifest")
                retained = len(data.get("artifacts", []))
                declared_paths = {entry.get("path") for entry in data.get("artifacts", []) if isinstance(entry, dict)}
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
                    path = bundle / entry.get("path", "")
                    actual_hash = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
                    inventory.append({"path": entry.get("path"), "sha256": entry.get("sha256"),
                                      "bytes": path.stat().st_size if path.is_file() else None,
                                      "hash_valid": actual_hash == str(entry.get("sha256", "")).lower()})
                missing_artifacts = sorted(item["path"] for item in inventory if item["bytes"] is None)
                mismatched_artifacts = sorted(item["path"] for item in inventory if not item["hash_valid"] and item["bytes"] is not None)
                scenario_inventory = sorted(
                    [{"id": item.get("id"), "status": item.get("status"), "evidence": item.get("evidence")}
                     for item in data.get("scenarios", []) if isinstance(item, dict)],
                    key=lambda item: str(item.get("id", "")))
                scenario_status_counts = dict(sorted(Counter(item["status"] for item in scenario_inventory).items()))
            except (OSError, json.JSONDecodeError) as exc:
                errors = [f"{platform}: invalid manifest: {exc}"]
                retained = 0
                artifact_bytes = 0
                manifest_sha256 = None
                inventory = []
                scenario_inventory = []
                scenario_status_counts = {}
                missing_artifacts = []
                mismatched_artifacts = []
        else:
            retained = 0
            artifact_bytes = 0
            manifest_sha256 = None
            inventory = []
            scenario_inventory = []
            scenario_status_counts = {}
            missing_artifacts = []
            mismatched_artifacts = []
            errors = [f"{platform}: missing manifest: {manifest}"]
        row_categories = Counter()
        for error in matrix_errors + errors:
            if "hash mismatch" in error: row_categories["hash"] += 1
            elif "missing artifact" in error or "missing manifest" in error: row_categories["retention"] += 1
            elif "stale" in error or "future" in error: row_categories["freshness"] += 1
            elif "path" in error or "escapes bundle" in error: row_categories["path"] += 1
            else: row_categories["manifest"] += 1
        rows.append({"platform": platform, "manifest": str(manifest), "manifest_sha256": manifest_sha256,
                     "retained_artifacts": retained, "retained_bytes": artifact_bytes,
                     "missing_artifacts": missing_artifacts, "mismatched_artifacts": mismatched_artifacts,
                     "artifact_inventory": inventory, "scenario_inventory": scenario_inventory,
                     "scenario_status_counts": scenario_status_counts,
                     "fresh": manifest_sha256 is not None and not any("stale" in error or "future" in error for error in identity_failures),
                     "freshness_errors": [error for error in identity_failures if "stale" in error or "future" in error or "timestamp" in error],
                     "manifest_platform": manifest_platform, "provenance_complete": provenance_complete,
                     "missing_provenance": missing_provenance,
                     "missing_scenarios": missing_scenarios,
                     "failure_categories": dict(sorted(row_categories.items())),
                     "error_count": len(matrix_errors + errors),
                     "retention_ok": not missing_artifacts and not mismatched_artifacts,
                     "invalidated": bool(matrix_errors + errors), "errors": matrix_errors + errors})
    errors = [error for row in rows for error in row["errors"]]
    categories = Counter()
    for error in errors:
        if "hash mismatch" in error: categories["hash"] += 1
        elif "escapes bundle" in error or "path" in error: categories["path"] += 1
        elif "stale" in error or "future" in error: categories["freshness"] += 1
        elif "missing artifact" in error or "missing manifest" in error: categories["retention"] += 1
        else: categories["manifest"] += 1
    return {"platforms": rows, "error_count": len(errors), "failure_categories": dict(sorted(categories.items())), "errors": errors,
            "ready": not errors}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--status", type=Path, required=True)
    parser.add_argument("--bundles", type=Path, required=True)
    parser.add_argument("--report", type=Path, help="write a JSON retention/readiness report")
    parser.add_argument("--max-age-hours", type=float, default=168, help="reject evidence older than this (default: 168)")
    args = parser.parse_args()
    errors = check(args.status, args.bundles, args.max_age_hours)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report(args.status, args.bundles, args.max_age_hours), indent=2) + "\n", encoding="utf-8")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"evidence readiness covers {len(platforms_from_status(args.status))} platform rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
