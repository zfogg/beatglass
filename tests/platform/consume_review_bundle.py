"""Consume a retained readiness report and its bound machine review packet."""
import argparse
import importlib.util
import json
from pathlib import Path


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + ".py"))
    module = importlib.util.module_from_spec(spec); assert spec.loader
    spec.loader.exec_module(module)
    return module


report_validator = _load("validate_readiness_report")
packet_consumer = _load("consume_review_packet")


def consume(bundle: Path, report_name: str = "readiness-report.json", packet_name: str = "review.json") -> list[str]:
    """Validate a retained report and packet as one reviewable artifact bundle."""
    report_path = bundle / report_name
    packet_path = bundle / packet_name
    errors = []
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid retained readiness report: {error}"]
    errors.extend(report_validator.validate(report))
    if not packet_path.is_file():
        errors.append(f"missing retained review packet: {packet_path.name}")
    else:
        errors.extend(packet_consumer.consume(packet_path, report_path))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--report-name", default="readiness-report.json")
    parser.add_argument("--packet-name", default="review.json")
    args = parser.parse_args()
    errors = consume(args.bundle, args.report_name, args.packet_name)
    if errors:
        print("\n".join(errors)); return 1
    print("review bundle accepted"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
