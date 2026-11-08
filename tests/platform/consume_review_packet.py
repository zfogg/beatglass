"""Consume a versioned machine review packet without accepting pending evidence."""
import argparse
import json
from pathlib import Path
import importlib.util

spec = importlib.util.spec_from_file_location("review", Path(__file__).with_name("review_readiness_handoff.py"))
review = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(review)


def consume(path: Path) -> list[str]:
    try:
        packet = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid review packet: {error}"]
    errors = review.validate_packet(packet)
    if not errors and packet.get("acceptance_errors"):
        errors.append("review packet contains acceptance errors")
    if packet.get("gate_status") == "READY" and packet.get("native_qualification_required") is True:
        errors.append("review packet requires native qualification")
    if packet.get("gate_status") == "READY" and any(row.get("status") == "experimental" for row in packet.get("platform_support_status", []) if isinstance(row, dict)):
        errors.append("review packet contains experimental platform support")
    if packet.get("gate_status") == "READY":
        unpassed = [f"{row.get('platform')}:{scenario.get('id')}" for row in packet.get("platforms", [])
                    if isinstance(row, dict) for scenario in row.get("scenarios", [])
                    if isinstance(scenario, dict) and scenario.get("status") != "PASS"]
        if unpassed:
            errors.append(f"review packet has unpassed scenarios: {unpassed}")
    if not errors and packet.get("gate_status") != "READY":
        errors.append(f"review packet is not accepted: {packet.get('gate_status')}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("packet", type=Path)
    args = parser.parse_args(); errors = consume(args.packet)
    if errors: print("\n".join(errors)); return 1
    print("review packet accepted"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
