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
