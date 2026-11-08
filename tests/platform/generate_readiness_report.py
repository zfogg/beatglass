"""Build a conservative readiness report from retained platform bundles.

The generator never upgrades native qualification: generated reports are
always PENDING until a separate, human-reviewed native gate changes them.
"""
import argparse
import hashlib
import json
from pathlib import Path

PLATFORMS = ("windows", "macos", "linux")
SCENARIOS = ("capture", "display", "output", "overlay", "playback", "recovery")


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_report(root: Path) -> dict:
    rows = []
    for platform in PLATFORMS:
        bundle = root / platform
        manifest_path = bundle / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        inventory = []
        for item in manifest.get("artifacts", []):
            path = bundle / str(item.get("path", ""))
            exists = path.is_file()
            digest = _sha(path) if exists else ""
            inventory.append({"path": item.get("path"), "sha256": digest,
                             "bytes": path.stat().st_size if exists else -1,
                             "hash_valid": exists and digest == item.get("sha256")})
        scenarios = {str(item.get("id")): item for item in manifest.get("scenarios", [])}
        scenario_inventory = [{"id": name, "status": scenarios.get(name, {}).get("status", "UNRUN"),
                               "evidence": scenarios.get(name, {}).get("evidence")} for name in SCENARIOS]
        rows.append({"platform": platform,
                     "manifest": str(manifest_path),
                     "manifest_sha256": _sha(manifest_path),
                     "retained_artifacts": len(inventory),
                     "retained_bytes": sum(max(0, x["bytes"]) for x in inventory),
                     "artifact_inventory": inventory,
                     "scenario_inventory": scenario_inventory,
                     "qualification_command": manifest.get("qualification_command", ""),
                     "qualification_output": manifest.get("qualification_output", ""),
                     "manifest_platform": manifest.get("platform"),
                     "provenance_complete": all(manifest.get(k) for k in ("platform", "commit", "captured_at", "environment")),
                     "errors": []})
    status = [{"platform": p, "status": "experimental"} for p in PLATFORMS]
    return {"readiness_report_schema": 1, "support_language": "experimental",
            "native_qualification_required": True, "exit_gate_status": "PENDING",
            "exit_gate_reason_codes": ["native_qualification_required", "generated_report_pending_review"],
            "ready": False, "platform_support_status": status, "platforms": rows}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundles", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.bundles)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"wrote conservative readiness report: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
