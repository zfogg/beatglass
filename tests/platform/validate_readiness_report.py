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
