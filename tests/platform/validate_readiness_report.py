"""Validate the stable, conservative machine-readable readiness report contract."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import sys

EXPECTED_PLATFORMS = {"windows", "macos", "linux"}
EXPECTED_SCENARIO_STATUSES = {"PASS", "FAIL", "UNRUN", "PENDING"}
REQUIRED_SCENARIOS = {"capture", "recovery", "output", "display", "playback", "overlay"}


def _valid_digest(value: object) -> bool:
    if not isinstance(value, str) or len(value) != 64:
        return False
    try:
        int(value, 16)
    except ValueError:
        return False
    return True


def _valid_commit(value: object) -> bool:
    if not isinstance(value, str) or len(value) != 40:
        return False
    try:
        int(value, 16)
    except ValueError:
        return False
    return True


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
    reason_codes = report.get("exit_gate_reason_codes")
    if (not isinstance(reason_codes, list)
            or any(not isinstance(code, str) or not code.strip() for code in reason_codes)
            or len(reason_codes) != len(set(reason_codes))):
        errors.append("exit_gate_reason_codes must be a list")
    statuses = report.get("platform_support_status")
    if not isinstance(statuses, list) or len(statuses) != len(EXPECTED_PLATFORMS) or any(not isinstance(item, dict) for item in statuses) or {item.get("platform") for item in statuses} != EXPECTED_PLATFORMS:
        errors.append("platform_support_status must contain exactly Windows, macOS, and Linux rows")
    elif any(item.get("status") != "experimental" for item in statuses):
        errors.append("platform_support_status rows must remain experimental")
    if report.get("exit_gate_status") == "READY" and report.get("exit_gate_reason_codes"):
        errors.append("READY report cannot contain exit_gate_reason_codes")
    if report.get("exit_gate_status") == "PENDING" and not report.get("exit_gate_reason_codes"):
        errors.append("PENDING report must contain exit_gate_reason_codes")
    if report.get("exit_gate_status") == "READY" and report.get("ready") is not True:
        errors.append("READY report must set ready true")
    if report.get("exit_gate_status") == "PENDING" and report.get("ready") is not False:
        errors.append("PENDING report must set ready false")
    generation_errors = report.get("generation_errors", [])
    if not isinstance(generation_errors, list) or any(not isinstance(error, str) or not error.strip() for error in generation_errors):
        errors.append("generation_errors must be a list of strings")
    elif generation_errors:
        errors.append("generation_errors must be empty before report consumption")
    platforms = report.get("platforms")
    if not isinstance(platforms, list) or len(platforms) != len(EXPECTED_PLATFORMS) or any(not isinstance(item, dict) for item in platforms) or {item.get("platform") for item in platforms} != EXPECTED_PLATFORMS:
        errors.append("platforms must contain exactly Windows, macOS, and Linux rows")
    else:
        for item in platforms:
            if not isinstance(item.get("manifest"), str) or not item["manifest"]:
                errors.append(f"{item.get('platform')}: missing manifest linkage")
            if not _valid_digest(item.get("manifest_sha256")):
                errors.append(f"{item.get('platform')}: invalid manifest hash linkage")
            if item.get("provenance_complete") is not True:
                errors.append(f"{item.get('platform')}: provenance is incomplete")
            if not isinstance(item.get("qualification_command"), str) or not item["qualification_command"].strip():
                errors.append(f"{item.get('platform')}: missing qualification command linkage")
            if not isinstance(item.get("qualification_output"), str) or not item["qualification_output"].strip():
                errors.append(f"{item.get('platform')}: missing qualification output linkage")
            if not _valid_digest(item.get("qualification_output_sha256")):
                errors.append(f"{item.get('platform')}: invalid qualification output hash")
            if not isinstance(item.get("qualification_output_bytes"), int) or item["qualification_output_bytes"] < 0:
                errors.append(f"{item.get('platform')}: invalid qualification output byte count")
            if not isinstance(item.get("retained_artifacts"), int) or item["retained_artifacts"] < 0:
                errors.append(f"{item.get('platform')}: invalid retained artifact count")
            if not isinstance(item.get("retained_bytes"), int) or item["retained_bytes"] < 0:
                errors.append(f"{item.get('platform')}: invalid retained byte count")
            scenario_inventory = item.get("scenario_inventory")
            scenario_ids = {str(entry.get("id")) for entry in scenario_inventory if isinstance(entry, dict)} if isinstance(scenario_inventory, list) else set()
            if not isinstance(scenario_inventory, list) or len(scenario_inventory) != len(REQUIRED_SCENARIOS) or scenario_ids != REQUIRED_SCENARIOS:
                errors.append(f"{item.get('platform')}: scenario inventory must contain exactly the required scenario set")
            evidence_hashes = item.get("scenario_evidence_sha256")
            if not isinstance(evidence_hashes, dict):
                errors.append(f"{item.get('platform')}: missing scenario evidence hashes")
            else:
                for scenario, digest in evidence_hashes.items():
                    if not _valid_digest(digest):
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
                    if not _valid_digest(artifact.get("sha256")):
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
        row_errors = row.get("errors", [])
        if not isinstance(row_errors, list) or any(not isinstance(error, str) or not error.strip() for error in row_errors):
            errors.append(f"{platform}: report row errors must be a list of strings")
        elif row_errors:
            errors.append(f"{platform}: report row contains generation errors")
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
            if field in row and row.get(field) != manifest_data.get(field):
                errors.append(f"{platform}: report provenance does not match manifest: {field}")
        if not _valid_commit(manifest_data.get("commit")):
            errors.append(f"{platform}: manifest commit must be a 40-character hexadecimal SHA-1")
        captured_at = manifest_data.get("captured_at")
        if isinstance(captured_at, str) and captured_at.strip():
            try:
                datetime.fromisoformat(captured_at.replace("Z", "+00:00"))
            except ValueError:
                errors.append(f"{platform}: manifest captured_at must be ISO-8601")
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
            for artifact in manifest_data["artifacts"]:
                if not isinstance(artifact, dict) or not isinstance(artifact.get("role"), str) or not artifact["role"].strip():
                    errors.append(f"{platform}: manifest artifact role is missing")
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
        report_inventory = row.get("artifact_inventory", [])
        report_paths = [item.get("path") for item in report_inventory if isinstance(item, dict)]
        manifest_paths = [item.get("path") for item in manifest_data.get("artifacts", []) if isinstance(item, dict)]
        if set(report_paths) != set(manifest_paths):
            errors.append(f"{platform}: report artifact inventory does not match manifest")
        if "retained_artifacts" in row and row.get("retained_artifacts") != len(manifest_paths):
            errors.append(f"{platform}: retained artifact count does not match manifest")
        if "retained_bytes" in row:
            actual_retained_bytes = sum(
                (bundles / platform / path).stat().st_size
                for path in manifest_paths
                if isinstance(path, str) and (bundles / platform / path).is_file()
            )
            if row.get("retained_bytes") != actual_retained_bytes:
                errors.append(f"{platform}: retained byte count does not match manifest")
        report_roles = {item.get("path"): item.get("role") for item in report_inventory if isinstance(item, dict) and item.get("role")}
        manifest_roles = {item.get("path"): item.get("role") for item in manifest_data.get("artifacts", []) if isinstance(item, dict)}
        if report_roles and report_roles != manifest_roles:
            errors.append(f"{platform}: report artifact roles do not match manifest")
        manifest_role_by_path = manifest_roles
        for artifact in manifest_data.get("artifacts", []):
            if not isinstance(artifact, dict) or not isinstance(artifact.get("path"), str):
                continue
            artifact_path = bundles / platform / artifact["path"]
            if artifact_path.resolve().is_relative_to(platform_root) and artifact_path.is_file():
                actual_artifact_hash = hashlib.sha256(artifact_path.read_bytes()).hexdigest()
                if actual_artifact_hash != str(artifact.get("sha256", "")).lower():
                    errors.append(f"{platform}: manifest artifact hash mismatch: {artifact['path']}")
        evidence_hashes = row.get("scenario_evidence_sha256", {})
        if not isinstance(evidence_hashes, dict):
            errors.append(f"{platform}: scenario evidence hashes must be an object")
            evidence_hashes = {}
        scenarios = manifest_data.get("scenarios", [])
        if not isinstance(scenarios, list):
            errors.append(f"{platform}: manifest scenarios must be an array")
            scenarios = []
        manifest_ids = {str(item.get("id")) for item in scenarios if isinstance(item, dict)}
        if manifest_ids != REQUIRED_SCENARIOS:
            errors.append(f"{platform}: manifest scenarios must contain exactly the required scenario set")
        report_scenarios = row.get("scenario_inventory", [])
        manifest_scenario_map = {
            str(item.get("id")): (item.get("status"), item.get("evidence"))
            for item in scenarios if isinstance(item, dict) and item.get("id") is not None
        }
        report_scenario_map = {
            str(item.get("id")): (item.get("status"), item.get("evidence"))
            for item in report_scenarios if isinstance(item, dict) and item.get("id") is not None
        } if isinstance(report_scenarios, list) else {}
        if report_scenario_map != manifest_scenario_map:
            errors.append(f"{platform}: scenario inventory does not match manifest")
        scenario_ids = set()
        expected_evidence_ids = set()
        for scenario in scenarios:
            if not isinstance(scenario, dict):
                errors.append(f"{platform}: manifest scenario entry must be an object")
                continue
            scenario_id = str(scenario.get("id"))
            if not scenario_id.strip() or scenario_id == "None":
                errors.append(f"{platform}: manifest scenario id is missing")
                continue
            if scenario.get("status") not in EXPECTED_SCENARIO_STATUSES:
                errors.append(f"{platform}: invalid manifest scenario status: {scenario.get('status')}")
            if scenario_id in scenario_ids:
                errors.append(f"{platform}: duplicate manifest scenario id: {scenario_id}")
            scenario_ids.add(scenario_id)
            evidence = scenario.get("evidence")
            status = scenario.get("status")
            if status in {"PASS", "FAIL"} and (not isinstance(evidence, str) or not evidence.strip()):
                errors.append(f"{platform}: completed scenario is missing evidence: {scenario_id}")
            if status in {"UNRUN", "PENDING"} and evidence:
                errors.append(f"{platform}: incomplete scenario must not claim evidence: {scenario_id}")
            if not evidence:
                continue
            expected_evidence_ids.add(scenario_id)
            if evidence not in manifest_paths:
                errors.append(f"{platform}: scenario evidence is not listed as an artifact: {evidence}")
            elif manifest_role_by_path.get(evidence) != scenario_id:
                errors.append(f"{platform}: scenario evidence role does not match scenario: {scenario_id}")
            path = bundles / platform / str(evidence)
            if not path.resolve().is_relative_to(platform_root):
                errors.append(f"{platform}: scenario evidence escapes retained bundle: {evidence}")
                continue
            if not path.is_file():
                errors.append(f"{platform}: retained scenario evidence is missing: {evidence}")
                continue
            if hashlib.sha256(path.read_bytes()).hexdigest() != evidence_hashes.get(scenario_id):
                errors.append(f"{platform}: scenario evidence hash does not match report: {scenario.get('id')}")
        if set(evidence_hashes) != expected_evidence_ids:
            errors.append(f"{platform}: scenario evidence hash mapping does not match manifest scenarios")
        manifest_platform = manifest_data.get("platform")
        if not isinstance(manifest_platform, str) or manifest_platform.lower() != platform:
            errors.append(f"{platform}: manifest platform does not match report row")
        if "manifest_platform" in row and row.get("manifest_platform") != manifest_platform:
            errors.append(f"{platform}: report manifest platform does not match manifest")
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
