"""Validate the stable, conservative machine-readable readiness report contract."""
import argparse
import json
from pathlib import Path
import sys

EXPECTED_PLATFORMS = {"windows", "macos", "linux"}


def validate(report: dict) -> list[str]:
    errors = []
    if report.get("readiness_report_schema") != 1:
        errors.append("unsupported readiness_report_schema")
    if report.get("support_language") != "experimental":
        errors.append("support_language must remain experimental")
    if report.get("native_qualification_required") is not True:
        errors.append("native_qualification_required must be true")
    if report.get("exit_gate_status") not in {"PENDING", "READY"}:
        errors.append("exit_gate_status must be PENDING or READY")
    if not isinstance(report.get("exit_gate_reason_codes"), list):
        errors.append("exit_gate_reason_codes must be a list")
    statuses = report.get("platform_support_status")
    if not isinstance(statuses, list) or {item.get("platform") for item in statuses if isinstance(item, dict)} != EXPECTED_PLATFORMS:
        errors.append("platform_support_status must contain exactly Windows, macOS, and Linux rows")
    if report.get("exit_gate_status") == "READY" and report.get("exit_gate_reason_codes"):
        errors.append("READY report cannot contain exit_gate_reason_codes")
    if report.get("exit_gate_status") == "PENDING" and not report.get("exit_gate_reason_codes"):
        errors.append("PENDING report must contain exit_gate_reason_codes")
    if report.get("exit_gate_status") == "READY" and report.get("ready") is not True:
        errors.append("READY report must set ready true")
    if report.get("exit_gate_status") == "PENDING" and report.get("ready") is not False:
        errors.append("PENDING report must set ready false")
    platforms = report.get("platforms")
    if not isinstance(platforms, list) or {item.get("platform") for item in platforms if isinstance(item, dict)} != EXPECTED_PLATFORMS:
        errors.append("platforms must contain exactly Windows, macOS, and Linux rows")
    else:
        for item in platforms:
            if not isinstance(item.get("manifest"), str) or not item["manifest"]:
                errors.append(f"{item.get('platform')}: missing manifest linkage")
            if not isinstance(item.get("manifest_sha256"), str) or len(item["manifest_sha256"]) != 64:
                errors.append(f"{item.get('platform')}: invalid manifest hash linkage")
            if item.get("provenance_complete") is not True:
                errors.append(f"{item.get('platform')}: provenance is incomplete")
            inventory = item.get("artifact_inventory")
            if not isinstance(inventory, list) or not inventory:
                errors.append(f"{item.get('platform')}: missing artifact inventory")
            else:
                for artifact in inventory:
                    if not isinstance(artifact, dict) or not artifact.get("path"):
                        errors.append(f"{item.get('platform')}: artifact missing path")
                    if not isinstance(artifact.get("sha256"), str) or len(artifact["sha256"]) != 64:
                        errors.append(f"{item.get('platform')}: artifact missing SHA-256")
                    if not isinstance(artifact.get("bytes"), int) or artifact["bytes"] < 0:
                        errors.append(f"{item.get('platform')}: artifact has invalid byte count")
                    if artifact.get("hash_valid") is not True:
                        errors.append(f"{item.get('platform')}: artifact hash is not valid")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    try:
        report = json.loads(args.report.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"invalid readiness report: {exc}", file=sys.stderr)
        return 1
    errors = validate(report)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("readiness report contract verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
