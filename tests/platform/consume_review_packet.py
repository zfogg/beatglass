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
