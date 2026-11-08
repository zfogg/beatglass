"""Check that every documented platform row has an attributable evidence bundle."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from datetime import datetime, timezone
import re

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
    for platform in platforms_from_status(status):
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
    for platform in platforms_from_status(status):
        bundle = bundles / platform
        manifest = bundle / "manifest.json"
        errors = []
        if manifest.is_file():
            try:
                data = json.loads(manifest.read_text(encoding="utf-8"))
                errors = [f"{platform}: {error}" for error in verifier.verify(bundle, manifest)]
                if str(data.get("platform", "")).lower() != platform:
                    errors.append(f"{platform}: manifest platform does not match status row")
                errors.extend(f"{platform}: {error}" for error in identity_errors(data, max_age_hours))
                retained = len(data.get("artifacts", []))
                artifact_bytes = sum((bundle / entry["path"]).stat().st_size for entry in data.get("artifacts", [])
                                     if isinstance(entry, dict) and isinstance(entry.get("path"), str)
                                     and (bundle / entry["path"]).is_file())
                manifest_sha256 = hashlib.sha256(manifest.read_bytes()).hexdigest()
            except (OSError, json.JSONDecodeError) as exc:
                errors = [f"{platform}: invalid manifest: {exc}"]
                retained = 0
                artifact_bytes = 0
                manifest_sha256 = None
        else:
            retained = 0
            artifact_bytes = 0
            manifest_sha256 = None
            errors = [f"{platform}: missing manifest: {manifest}"]
        rows.append({"platform": platform, "manifest": str(manifest), "manifest_sha256": manifest_sha256,
                     "retained_artifacts": retained, "retained_bytes": artifact_bytes, "errors": errors})
    return {"platforms": rows, "ready": not any(row["errors"] for row in rows)}


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
