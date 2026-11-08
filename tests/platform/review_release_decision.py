"""Create a conservative machine-readable release decision from a retained bundle."""
import argparse
import hashlib
import json
from pathlib import Path
import importlib.util

spec = importlib.util.spec_from_file_location("bundle", Path(__file__).with_name("consume_review_bundle.py"))
bundle = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(bundle)


def decide(bundle_root: Path, report_name: str = "readiness-report.json", packet_name: str = "review.json") -> dict:
    report_path = bundle_root / "readiness-report.json"
    report_path = bundle_root / report_name
    errors = bundle.consume(bundle_root, report_name, packet_name)
    result = {
        "release_decision_schema": 1,
        "decision": "ACCEPTED" if not errors else "PENDING",
        "reasons": errors,
        "native_qualification_required": True,
    }
    try:
        result["report_sha256"] = hashlib.sha256(report_path.read_bytes()).hexdigest()
    except OSError:
        result["report_sha256"] = None
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = decide(args.bundle)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"release decision: {result['decision']}")
    return 0 if result["decision"] == "ACCEPTED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
