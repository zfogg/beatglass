"""Validate the stable, conservative machine-readable readiness report contract."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

EXPECTED_PLATFORMS = {"windows", "macos", "linux"}


def validate(report: dict) -> list[str]:
    if not isinstance(report, dict):
        return ["readiness report must be a JSON object"]
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
    if not isinstance(statuses, list) or len(statuses) != len(EXPECTED_PLATFORMS) or any(not isinstance(item, dict) for item in statuses) or {item.get("platform") for item in statuses} != EXPECTED_PLATFORMS:
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
    if not isinstance(platforms, list) or len(platforms) != len(EXPECTED_PLATFORMS) or any(not isinstance(item, dict) for item in platforms) or {item.get("platform") for item in platforms} != EXPECTED_PLATFORMS:
        errors.append("platforms must contain exactly Windows, macOS, and Linux rows")
    else:
        for item in platforms:
            if not isinstance(item.get("manifest"), str) or not item["manifest"]:
                errors.append(f"{item.get('platform')}: missing manifest linkage")
            if not isinstance(item.get("manifest_sha256"), str) or len(item["manifest_sha256"]) != 64:
                errors.append(f"{item.get('platform')}: invalid manifest hash linkage")
            if item.get("provenance_complete") is not True:
                errors.append(f"{item.get('platform')}: provenance is incomplete")
            if not isinstance(item.get("qualification_command"), str) or not item["qualification_command"].strip():
                errors.append(f"{item.get('platform')}: missing qualification command linkage")
            if not isinstance(item.get("qualification_output"), str) or not item["qualification_output"].strip():
                errors.append(f"{item.get('platform')}: missing qualification output linkage")
            if not isinstance(item.get("qualification_output_sha256"), str) or len(item["qualification_output_sha256"]) != 64:
                errors.append(f"{item.get('platform')}: invalid qualification output hash")
            if not isinstance(item.get("qualification_output_bytes"), int) or item["qualification_output_bytes"] < 0:
                errors.append(f"{item.get('platform')}: invalid qualification output byte count")
            evidence_hashes = item.get("scenario_evidence_sha256")
            if not isinstance(evidence_hashes, dict):
                errors.append(f"{item.get('platform')}: missing scenario evidence hashes")
            else:
                for scenario, digest in evidence_hashes.items():
                    if not isinstance(digest, str) or len(digest) != 64:
                        errors.append(f"{item.get('platform')}: invalid evidence hash for {scenario}")
            inventory = item.get("artifact_inventory")
            if not isinstance(inventory, list) or not inventory:
                errors.append(f"{item.get('platform')}: missing artifact inventory")
            else:
                inventory_paths = [artifact.get("path") for artifact in inventory if isinstance(artifact, dict)]
                if len(inventory_paths) != len(set(inventory_paths)):
                    errors.append(f"{item.get('platform')}: duplicate report artifact path")
                for artifact in inventory:
                    if not isinstance(artifact, dict):
                        errors.append(f"{item.get('platform')}: artifact entry must be an object")
                        continue
                    if not artifact.get("path"):
                        errors.append(f"{item.get('platform')}: artifact missing path")
                    if not isinstance(artifact.get("sha256"), str) or len(artifact["sha256"]) != 64:
                        errors.append(f"{item.get('platform')}: artifact missing SHA-256")
                    if not isinstance(artifact.get("bytes"), int) or artifact["bytes"] < 0:
                        errors.append(f"{item.get('platform')}: artifact has invalid byte count")
                    if artifact.get("hash_valid") is not True:
                        errors.append(f"{item.get('platform')}: artifact hash is not valid")
    return errors


def validate_retained_files(report: dict, bundles: Path) -> list[str]:
    if not isinstance(report, dict):
        return ["readiness report must be a JSON object"]
    errors = []
    for row in report.get("platforms", []):
        platform = row.get("platform")
        platform_root = (bundles / str(platform)).resolve()
        manifest = bundles / platform / "manifest.json"
        if not manifest.is_file():
            errors.append(f"{platform}: retained manifest is missing")
            continue
        try:
            manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            errors.append(f"{platform}: retained manifest is invalid JSON: {error}")
            continue
        for field in ("platform", "commit", "captured_at", "environment"):
            if not isinstance(manifest_data.get(field), str) or not manifest_data[field].strip():
                errors.append(f"{platform}: manifest provenance field missing: {field}")
        if not isinstance(manifest_data.get("qualification_command"), str) or not manifest_data["qualification_command"].strip():
            errors.append(f"{platform}: manifest qualification command is missing")
        elif row.get("qualification_command") != manifest_data["qualification_command"]:
            errors.append(f"{platform}: qualification command does not match report")
        output = manifest_data.get("qualification_output")
        if not isinstance(output, str) or not output.strip():
            errors.append(f"{platform}: manifest qualification output is missing")
        elif row.get("qualification_output") != output:
            errors.append(f"{platform}: qualification output does not match report")
        elif not isinstance(manifest_data.get("artifacts"), list):
            errors.append(f"{platform}: manifest artifacts must be an array")
        else:
            artifact_paths = [item.get("path") for item in manifest_data["artifacts"] if isinstance(item, dict)]
            if len(artifact_paths) != len(set(artifact_paths)):
                errors.append(f"{platform}: duplicate manifest artifact path")
            if not any(path == output for path in artifact_paths):
                errors.append(f"{platform}: qualification output is not retained as an artifact")
        if isinstance(output, str) and output.strip() and row.get("qualification_output") == output:
            output_path = bundles / platform / output
            if not output_path.resolve().is_relative_to(platform_root):
                errors.append(f"{platform}: qualification output escapes retained bundle")
                continue
            if output_path.is_file() and hashlib.sha256(output_path.read_bytes()).hexdigest() != row.get("qualification_output_sha256"):
                errors.append(f"{platform}: qualification output hash does not match report")
            if output_path.is_file() and output_path.stat().st_size != row.get("qualification_output_bytes"):
                errors.append(f"{platform}: qualification output byte count does not match report")
        evidence_hashes = row.get("scenario_evidence_sha256", {})
        if not isinstance(evidence_hashes, dict):
            errors.append(f"{platform}: scenario evidence hashes must be an object")
            evidence_hashes = {}
        scenarios = manifest_data.get("scenarios", [])
        if not isinstance(scenarios, list):
            errors.append(f"{platform}: manifest scenarios must be an array")
            scenarios = []
        scenario_ids = set()
        for scenario in scenarios:
            if not isinstance(scenario, dict):
                errors.append(f"{platform}: manifest scenario entry must be an object")
                continue
            scenario_id = str(scenario.get("id"))
            if not scenario_id.strip() or scenario_id == "None":
                errors.append(f"{platform}: manifest scenario id is missing")
                continue
            if scenario_id in scenario_ids:
                errors.append(f"{platform}: duplicate manifest scenario id: {scenario_id}")
            scenario_ids.add(scenario_id)
            evidence = scenario.get("evidence")
            if not evidence:
                continue
            path = bundles / platform / str(evidence)
            if not path.resolve().is_relative_to(platform_root):
                errors.append(f"{platform}: scenario evidence escapes retained bundle: {evidence}")
                continue
            if not path.is_file():
                errors.append(f"{platform}: retained scenario evidence is missing: {evidence}")
                continue
            if hashlib.sha256(path.read_bytes()).hexdigest() != evidence_hashes.get(scenario_id):
                errors.append(f"{platform}: scenario evidence hash does not match report: {scenario.get('id')}")
        manifest_platform = manifest_data.get("platform")
        if not isinstance(manifest_platform, str) or manifest_platform.lower() != platform:
            errors.append(f"{platform}: manifest platform does not match report row")
        actual_manifest_hash = hashlib.sha256(manifest.read_bytes()).hexdigest()
        if actual_manifest_hash != row.get("manifest_sha256"):
            errors.append(f"{platform}: manifest SHA-256 does not match report")
        for artifact in row.get("artifact_inventory", []):
            path = bundles / platform / str(artifact.get("path", ""))
            if not path.resolve().is_relative_to(platform_root):
                errors.append(f"{platform}: retained artifact escapes bundle: {artifact.get('path')}")
                continue
            if not path.is_file():
                errors.append(f"{platform}: retained artifact is missing: {artifact.get('path')}")
                continue
            actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
            if actual_hash != str(artifact.get("sha256", "")).lower():
                errors.append(f"{platform}: retained artifact hash mismatch: {artifact.get('path')}")
            if path.stat().st_size != artifact.get("bytes"):
                errors.append(f"{platform}: retained artifact byte count mismatch: {artifact.get('path')}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("report", type=Path)
    parser.add_argument("--bundles", type=Path, help="retained bundle root for manifest/artifact verification")
    args = parser.parse_args()
    try:
        report = json.loads(args.report.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"invalid readiness report: {exc}", file=sys.stderr)
        return 1
    errors = validate(report)
    if args.bundles:
        errors.extend(validate_retained_files(report, args.bundles))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("readiness report contract verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
