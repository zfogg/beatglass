"""Export and verify a readiness report/review packet retention bundle."""
import argparse
import importlib.util
import shutil
from pathlib import Path


spec = importlib.util.spec_from_file_location("bundle", Path(__file__).with_name("consume_review_bundle.py"))
bundle = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(bundle)


def retain(report: Path, packet: Path, destination: Path) -> list[str]:
    """Copy a bound report/packet pair and verify the retained copy."""
    if not report.is_file():
        return [f"missing source report: {report}"]
    if not packet.is_file():
        return [f"missing source review packet: {packet}"]
    destination.mkdir(parents=True, exist_ok=True)
    retained_report = destination / "readiness-report.json"
    retained_packet = destination / "review.json"
    try:
        shutil.copyfile(report, retained_report)
        shutil.copyfile(packet, retained_packet)
    except OSError as error:
        return [f"unable to retain review bundle: {error}"]
    errors = bundle.consume(destination)
    if errors:
        return [f"retained review bundle rejected: {error}" for error in errors]
    return []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("packet", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    errors = retain(args.report, args.packet, args.destination)
    if errors:
        print("\n".join(errors)); return 1
    print(f"retained and verified review bundle: {args.destination}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
