"""Promote a retained review bundle only after an accepted release decision."""
import argparse
import importlib.util
import shutil
from pathlib import Path

spec = importlib.util.spec_from_file_location("decision", Path(__file__).with_name("review_release_decision.py"))
decision = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(decision)


def promote(source: Path, destination: Path) -> tuple[bool, list[str]]:
    """Copy the canonical report/packet pair only when the bundle is accepted."""
    result = decision.decide(source)
    if result["decision"] != "ACCEPTED":
        return False, result["reasons"] or ["release decision is PENDING"]
    destination.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copyfile(source / "readiness-report.json", destination / "readiness-report.json")
        shutil.copyfile(source / "review.json", destination / "review.json")
    except OSError as error:
        return False, [f"unable to promote review bundle: {error}"]
    return True, []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    accepted, errors = promote(args.source, args.destination)
    if not accepted:
        print("release bundle not promoted: PENDING")
        print("\n".join(errors)); return 1
    print(f"promoted accepted review bundle: {args.destination}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
