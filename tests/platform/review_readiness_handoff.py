"""Render a human-review checklist from a conservative readiness report."""
import argparse
import json
from pathlib import Path
import importlib.util
import hashlib

consumer_spec = importlib.util.spec_from_file_location("consume_report", Path(__file__).with_name("consume_readiness_handoff.py"))
consumer = importlib.util.module_from_spec(consumer_spec); assert consumer_spec.loader
consumer_spec.loader.exec_module(consumer)


def render(report_path: Path) -> str:
    report = json.loads(report_path.read_text(encoding="utf-8"))
    lines = ["# Native qualification review", "", f"Gate status: **{report.get('exit_gate_status', 'UNKNOWN')}**",
             f"Native qualification required: **{report.get('native_qualification_required', 'UNKNOWN')}**", "",
             "This checklist is review evidence only; it does not promote native qualification.", ""]
    acceptance_errors = consumer.consume(report_path)
    lines.append("## Acceptance diagnostics")
    lines.extend([f"- {error}" for error in acceptance_errors] or ["- consumer validation passed"])
    lines.append("")
    lines.append("## Platform support")
    lines.extend([f"- {row.get('platform')}: {row.get('status')}" for row in report.get("platform_support_status", [])] or ["- none recorded"])
    lines.append("")
    for field in ("generation_errors", "handoff_check_errors", "documentation_check_errors", "ingestion_errors"):
        values = report.get(field, [])
        lines.append(f"## {field}")
        lines.extend([f"- {value}" for value in values] or ["- none recorded"])
        lines.append("")
    for row in report.get("platforms", []):
        lines.extend([f"## {row.get('platform', 'unknown')}", ""])
        lines.extend([f"- Manifest: `{row.get('manifest')}`", f"- Manifest SHA-256: `{row.get('manifest_sha256')}`",
                      f"- Source commit: `{row.get('commit')}`", f"- Captured at: `{row.get('captured_at')}`",
                      f"- Environment: `{row.get('environment')}`", f"- Qualification command: `{row.get('qualification_command')}`",
                      f"- Qualification output: `{row.get('qualification_output')}`", ""])
        for scenario in row.get("scenario_inventory", []):
            lines.append(f"- [{'x' if scenario.get('status') == 'PASS' else ' '}] {scenario.get('id')}: {scenario.get('status')}")
        lines.append("")
        lines.append("Artifacts:")
        for artifact in row.get("artifact_inventory", []):
            lines.append(f"- `{artifact.get('path')}` ({artifact.get('role', 'unowned')})")
        lines.append("")
    return "\n".join(lines) + "\n"


def review_packet(report_path: Path) -> dict:
    report = json.loads(report_path.read_text(encoding="utf-8"))
    return {"review_packet_schema": 1, "report_sha256": hashlib.sha256(report_path.read_bytes()).hexdigest(),
            "gate_status": report.get("exit_gate_status"),
            "native_qualification_required": report.get("native_qualification_required"),
            "acceptance_errors": consumer.consume(report_path),
            "platform_support_status": report.get("platform_support_status", []),
            "platforms": [{"platform": row.get("platform"), "manifest": row.get("manifest"),
                           "manifest_sha256": row.get("manifest_sha256"),
                           "scenarios": row.get("scenario_inventory", []),
                           "artifacts": row.get("artifact_inventory", [])}
                          for row in report.get("platforms", [])]}


def validate_packet(packet: dict) -> list[str]:
    errors = []
    if not isinstance(packet, dict) or packet.get("review_packet_schema") != 1:
        errors.append("unsupported review_packet_schema")
    if isinstance(packet, dict) and not isinstance(packet.get("acceptance_errors"), list):
        errors.append("review packet acceptance_errors must be a list")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path); parser.add_argument("output", type=Path)
    parser.add_argument("--require-ready", action="store_true")
    parser.add_argument("--json-output", type=Path, help="write a machine-readable review packet")
    args = parser.parse_args(); args.output.write_text(render(args.report), encoding="utf-8")
    report = json.loads(args.report.read_text(encoding="utf-8"))
    if args.require_ready and report.get("exit_gate_status") != "READY":
        print(f"review gate is not ready: {report.get('exit_gate_status')}"); return 1
    if args.require_ready:
        errors = consumer.consume(args.report)
        if errors:
            print("\n".join(errors)); return 1
    if args.json_output is not None:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json.dumps(review_packet(args.report), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"wrote native qualification review checklist: {args.output}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
