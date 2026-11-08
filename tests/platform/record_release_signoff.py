"""Record a human release signoff only for an accepted decision artifact."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path


def signoff(decision_path: Path, reviewer: str, output: Path) -> list[str]:
    try:
        decision = json.loads(decision_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid release decision: {error}"]
    if decision.get("release_decision_schema") != 1:
        return ["unsupported release decision schema"]
    if decision.get("decision") != "ACCEPTED":
        return ["cannot sign off a non-ACCEPTED release decision"]
    if not reviewer.strip():
        return ["reviewer identity is required"]
    signoff_record = {
        "release_signoff_schema": 1,
        "decision": "ACCEPTED",
        "reviewer": reviewer.strip(),
        "signed_at": datetime.now(timezone.utc).isoformat(),
        "report_sha256": decision.get("report_sha256"),
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(signoff_record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("decision", type=Path); parser.add_argument("reviewer"); parser.add_argument("output", type=Path)
    args = parser.parse_args(); errors = signoff(args.decision, args.reviewer, args.output)
    if errors:
        print("\n".join(errors)); return 1
    print(f"release signoff recorded: {args.output}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
