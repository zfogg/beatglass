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
manifest_verifier = _load("verify_release_manifest")


def prepare(report: Path, packet: Path, retained: Path, release: Path, signoff: Path | None = None) -> list[str]:
    errors = retainer.retain(report, packet, retained)
    if errors:
        return errors
    result = decision.decide(retained)
    if result["decision"] != "ACCEPTED":
        return [f"release decision is {result['decision']}"] + result["reasons"]
    if signoff is not None:
        try:
            signoff_data = __import__("json").loads(signoff.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            return [f"invalid release signoff: {error}"]
        if signoff_data.get("report_sha256") != result.get("report_sha256"):
            return ["release signoff report hash does not match decision"]
        if signoff_data.get("review_packet_sha256") != result.get("review_packet_sha256"):
            return ["release signoff packet hash does not match decision"]
    accepted, promotion_errors = promoter.promote(retained, release)
    if not accepted:
        return promotion_errors
    if signoff is not None:
        return manifest_verifier.verify(release, signoff)
    return []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path); parser.add_argument("packet", type=Path)
    parser.add_argument("retained", type=Path); parser.add_argument("release", type=Path)
    parser.add_argument("--signoff", type=Path, help="require and verify manual signoff")
    args = parser.parse_args(); errors = prepare(args.report, args.packet, args.retained, args.release, args.signoff)
    if errors:
        print("\n".join(errors)); return 1
    print(f"prepared and promoted release bundle: {args.release}"); return 0


if __name__ == "__main__":
    raise SystemExit(main())
