"""Run retention, decision, and promotion as one fail-closed release workflow."""
import argparse
import importlib.util
from pathlib import Path


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + ".py"))
    module = importlib.util.module_from_spec(spec); assert spec.loader
    spec.loader.exec_module(module)
    return module


retainer = _load("retain_review_bundle")
decision = _load("review_release_decision")
promoter = _load("promote_review_bundle")


def prepare(report: Path, packet: Path, retained: Path, release: Path) -> list[str]:
    errors = retainer.retain(report, packet, retained)
    if errors:
        return errors
    result = decision.decide(retained)
    if result["decision"] != "ACCEPTED":
        return [f"release decision is {result['decision']}"] + result["reasons"]
    accepted, promotion_errors = promoter.promote(retained, release)
    return [] if accepted else promotion_errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path); parser.add_argument("packet", type=Path)
    parser.add_argument("retained", type=Path); parser.add_argument("release", type=Path)
    args = parser.parse_args(); errors = prepare(args.report, args.packet, args.retained, args.release)
    if errors:
        print("\n".join(errors)); return 1
    print(f"prepared and promoted release bundle: {args.release}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
