"""Render a human-review checklist from a conservative readiness report."""
import argparse
import json
from pathlib import Path


def render(report_path: Path) -> str:
    report = json.loads(report_path.read_text(encoding="utf-8"))
    lines = ["# Native qualification review", "", f"Gate status: **{report.get('exit_gate_status', 'UNKNOWN')}**", "",
             "This checklist is review evidence only; it does not promote native qualification.", ""]
    for field in ("generation_errors", "handoff_check_errors", "documentation_check_errors", "ingestion_errors"):
        values = report.get(field, [])
        lines.append(f"## {field}")
        lines.extend([f"- {value}" for value in values] or ["- none recorded"])
        lines.append("")
    for row in report.get("platforms", []):
        lines.extend([f"## {row.get('platform', 'unknown')}", ""])
        for scenario in row.get("scenario_inventory", []):
            lines.append(f"- [{'x' if scenario.get('status') == 'PASS' else ' '}] {scenario.get('id')}: {scenario.get('status')}")
        lines.append("")
        lines.append("Artifacts:")
        for artifact in row.get("artifact_inventory", []):
            lines.append(f"- `{artifact.get('path')}` ({artifact.get('role', 'unowned')})")
        lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path); parser.add_argument("output", type=Path)
    parser.add_argument("--require-ready", action="store_true")
    args = parser.parse_args(); args.output.write_text(render(args.report), encoding="utf-8")
    report = json.loads(args.report.read_text(encoding="utf-8"))
    if args.require_ready and report.get("exit_gate_status") != "READY":
        print(f"review gate is not ready: {report.get('exit_gate_status')}"); return 1
    print(f"wrote native qualification review checklist: {args.output}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
