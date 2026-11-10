"""Build a conservative readiness report from retained platform bundles.

The generator never upgrades native qualification: generated reports are
always PENDING until a separate, human-reviewed native gate changes them.
"""
import argparse
import hashlib
import json
import os
import tempfile
from pathlib import Path

PLATFORMS = ("windows", "macos", "linux")
MAX_REPORT_BYTES = 16 << 20
SCENARIOS = ("capture", "display", "output", "overlay", "playback", "recovery")


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_report(root: Path) -> dict:
    rows = []
    for platform in PLATFORMS:
        bundle = root / platform
        bundle_root = bundle.resolve()
        errors = []
        manifest_path = bundle / "manifest.json"
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            manifest = {}
            errors.append(f"invalid manifest JSON: {error}")
        if not isinstance(manifest, dict):
            manifest = {}
            errors.append("manifest root must be an object")
        artifacts = manifest.get("artifacts", [])
        if not isinstance(artifacts, list):
            errors.append("manifest artifacts must be an array")
            artifacts = []
        scenarios_data = manifest.get("scenarios", [])
        if not isinstance(scenarios_data, list):
            errors.append("manifest scenarios must be an array")
            scenarios_data = []
        inventory = []
        for item in artifacts:
            if not isinstance(item, dict):
                errors.append(f"invalid artifact entry: {item}")
                continue
            path = bundle / str(item.get("path", ""))
            if not path.resolve().is_relative_to(bundle_root):
                errors.append(f"artifact escapes bundle: {item.get('path')}")
                inventory.append({"path": item.get("path"), "sha256": "", "bytes": -1, "hash_valid": False})
                continue
            exists = path.is_file()
            digest = _sha(path) if exists else ""
            if not exists:
                errors.append(f"missing artifact: {item.get('path')}")
            inventory.append({"path": item.get("path"), "sha256": digest,
                             "role": item.get("role"),
                             "bytes": path.stat().st_size if exists else -1,
                             "hash_valid": exists and digest == item.get("sha256")})
        scenarios = {str(item.get("id")): item for item in scenarios_data if isinstance(item, dict)}
        scenario_inventory = [{"id": name, "status": scenarios.get(name, {}).get("status", "UNRUN"),
                               "evidence": scenarios.get(name, {}).get("evidence")} for name in SCENARIOS]
        output_name = manifest.get("qualification_output", "")
        if not isinstance(output_name, str):
            errors.append("qualification output must be a string")
            output_name = ""
        output_path = bundle / output_name
        if output_name and not output_path.resolve().is_relative_to(bundle_root):
            errors.append(f"qualification output escapes bundle: {output_name}")
            output_path = bundle / "__missing_output__"
        elif output_name and not output_path.is_file():
            errors.append(f"missing qualification output: {output_name}")
        output_hash = _sha(output_path) if output_path.is_file() else ""
        evidence_hashes = {}
        for scenario in scenario_inventory:
            evidence = scenario.get("evidence")
            if evidence:
                evidence_path = bundle / str(evidence)
                if not evidence_path.resolve().is_relative_to(bundle_root):
                    errors.append(f"scenario evidence escapes bundle: {evidence}")
                    evidence_hashes[scenario["id"]] = ""
                    continue
                if not evidence_path.is_file():
                    errors.append(f"missing scenario evidence: {evidence}")
                evidence_hashes[scenario["id"]] = _sha(evidence_path) if evidence_path.is_file() else ""
        rows.append({"platform": platform,
                     "manifest": str(manifest_path),
                     "manifest_sha256": _sha(manifest_path) if manifest_path.is_file() else "",
                     "retained_artifacts": len(inventory),
                     "retained_bytes": sum(max(0, x["bytes"]) for x in inventory),
                     "artifact_inventory": inventory,
                     "scenario_inventory": scenario_inventory,
                     "qualification_command": manifest.get("qualification_command", ""),
                     "qualification_output": manifest.get("qualification_output", ""),
                     "qualification_output_sha256": output_hash,
                     "qualification_output_bytes": output_path.stat().st_size if output_path.is_file() else -1,
                     "scenario_evidence_sha256": evidence_hashes,
                     "manifest_platform": manifest.get("platform"),
                     "commit": manifest.get("commit", ""),
                     "captured_at": manifest.get("captured_at", ""),
                     "environment": manifest.get("environment", ""),
                     "provenance_complete": all(manifest.get(k) for k in ("platform", "commit", "captured_at", "environment")),
                     "errors": errors})
    status = [{"platform": p, "status": "experimental"} for p in PLATFORMS]
    generation_errors = [f"{row['platform']}: {error}" for row in rows for error in row.get("errors", [])]
    return {"readiness_report_schema": 1, "support_language": "experimental",
            "native_qualification_required": True, "exit_gate_status": "PENDING",
            "exit_gate_reason_codes": ["native_qualification_required", "generated_report_pending_review"],
            "ready": False, "platform_support_status": status, "platforms": rows,
            "generation_errors": generation_errors, "handoff_check_errors": [],
            "documentation_check_errors": [], "ingestion_errors": []}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundles", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--max-report-bytes", type=int, default=MAX_REPORT_BYTES)
    parser.add_argument("--json", action="store_true", help="emit the generated report on stdout")
    args = parser.parse_args()
    report = build_report(args.bundles)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    serialized = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.max_report_bytes <= 0:
        print("report size limit must be positive")
        return 1
    if len(serialized.encode("utf-8")) > args.max_report_bytes:
        print(f"readiness report exceeds {args.max_report_bytes} bytes")
        return 1
    temporary = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=args.output.parent,
                                         prefix=f".{args.output.name}.", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name); stream.write(serialized); stream.flush(); os.fsync(stream.fileno())
        os.replace(temporary, args.output); temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    if args.json:
        print(serialized, end="")
    else:
        print(f"wrote conservative readiness report: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
