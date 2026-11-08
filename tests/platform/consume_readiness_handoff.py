"""Consume a readiness handoff without upgrading pending evidence."""
import argparse
import importlib.util
import json
from pathlib import Path

spec = importlib.util.spec_from_file_location("validate_report", Path(__file__).with_name("validate_readiness_report.py"))
validator = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(validator)


def consume(report_path: Path) -> list[str]:
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid readiness report: {error}"]
    errors = validator.validate(report)
    for field in ("generation_errors", "handoff_check_errors", "documentation_check_errors", "ingestion_errors"):
        if report.get(field):
            errors.append(f"readiness handoff contains {field}")
    if not errors and report.get("exit_gate_status") == "READY" and report.get("native_qualification_required") is True:
        errors.append("readiness handoff cannot be READY while native qualification is required")
    if report.get("exit_gate_status") == "READY":
        if any(isinstance(row, dict) and row.get("status") == "experimental"
               for row in report.get("platform_support_status", [])):
            errors.append("readiness handoff cannot be READY with experimental platform support")
        unpassed = [f"{row.get('platform')}:{entry.get('id')}" for row in report.get("platforms", [])
                    if isinstance(row, dict) for entry in row.get("scenario_inventory", [])
                    if isinstance(entry, dict) and entry.get("status") != "PASS"]
        if unpassed:
            errors.append(f"readiness handoff has unpassed scenarios: {unpassed}")
        missing_roles = [entry.get("path") for row in report.get("platforms", [])
                         for entry in row.get("artifact_inventory", [])
                         if not isinstance(entry, dict) or not isinstance(entry.get("role"), str) or not entry["role"]]
        if missing_roles:
            errors.append(f"readiness handoff has unowned artifact roles: {missing_roles}")
    if not errors and report.get("exit_gate_status") != "READY":
        errors.append(f"readiness handoff is not accepted: {report.get('exit_gate_status')}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    errors = consume(args.report)
    if errors:
        print("\n".join(errors))
        return 1
    print("readiness handoff accepted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
