"""Promote a retained review bundle only after an accepted release decision."""
import argparse
import hashlib
import importlib.util
import json
import shutil
from pathlib import Path

spec = importlib.util.spec_from_file_location("decision", Path(__file__).with_name("review_release_decision.py"))
decision = importlib.util.module_from_spec(spec); assert spec.loader
spec.loader.exec_module(decision)
verify_spec = importlib.util.spec_from_file_location("manifest_verifier", Path(__file__).with_name("verify_release_manifest.py"))
manifest_verifier = importlib.util.module_from_spec(verify_spec); assert verify_spec.loader
verify_spec.loader.exec_module(manifest_verifier)


def promote(source: Path, destination: Path) -> tuple[bool, list[str]]:
    """Copy the canonical report/packet pair only when the bundle is accepted."""
    result = decision.decide(source)
    if result["decision"] != "ACCEPTED":
        return False, result["reasons"] or ["release decision is PENDING"]
    destination.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copyfile(source / "readiness-report.json", destination / "readiness-report.json")
        shutil.copyfile(source / "review.json", destination / "review.json")
        manifest = {
            "release_manifest_schema": 1,
            "decision": "ACCEPTED",
            "native_qualification_status": "pending",
            "report_sha256": hashlib.sha256((destination / "readiness-report.json").read_bytes()).hexdigest(),
            "review_packet_sha256": hashlib.sha256((destination / "review.json").read_bytes()).hexdigest(),
        }
        (destination / "release-manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        verification_errors = manifest_verifier.verify(destination)
        if verification_errors:
            return False, [f"promoted release manifest rejected: {error}" for error in verification_errors]
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
