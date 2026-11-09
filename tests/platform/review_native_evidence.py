"""Create a conservative reviewer assessment from retained native evidence."""
import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

REQUIRED = {"capture", "recovery", "output", "display", "playback", "overlay"}


def assess(report_path: Path, reviewer: str) -> dict:
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return {"native_review_schema": 1, "decision": "PENDING", "reviewer": reviewer.strip(), "blockers": [str(error)]}
    blockers = []
    for platform in report.get("platforms", []):
        name = platform.get("platform", "unknown")
        scenarios = {row.get("id"): row for row in platform.get("scenarios", []) if isinstance(row, dict)}
        for scenario in sorted(REQUIRED):
            row = scenarios.get(scenario)
            if not row or row.get("status") != "PASS" or not isinstance(row.get("evidence"), str) or not row["evidence"].strip():
                blockers.append(f"{name}:{scenario}")
    if not reviewer.strip():
        blockers.append("reviewer identity is required")
    return {"native_review_schema": 1, "decision": "ACCEPTED" if not blockers else "PENDING",
            "reviewer": reviewer.strip(), "reviewed_at": datetime.now(timezone.utc).isoformat(),
            "report": str(report_path), "blockers": blockers}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("report", type=Path); parser.add_argument("reviewer"); parser.add_argument("output", type=Path)
    args = parser.parse_args(); result = assess(args.report, args.reviewer)
    args.output.parent.mkdir(parents=True, exist_ok=True); args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"native evidence review: {result['decision']}"); return 0 if result["decision"] == "ACCEPTED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
