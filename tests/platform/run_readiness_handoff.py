"""Generate and validate one conservative readiness handoff report."""
import argparse
import importlib.util
import json
from pathlib import Path


def _load(name: str):
    path = Path(__file__).with_name(f"{name}.py")
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec); assert spec.loader
    spec.loader.exec_module(module)
    return module


generator = _load("generate_readiness_report")
validator = _load("validate_readiness_report")
readiness = _load("check_evidence_readiness")
handoff_docs = _load("check_qualification_handoff")
consumer = _load("consume_readiness_handoff")
ingester = _load("ingest_qualification_artifacts")
reviewer = _load("review_readiness_handoff")
packet_consumer = _load("consume_review_packet")
bundle_consumer = _load("consume_review_bundle")
decision_consumer = _load("review_release_decision")


def run(bundles: Path, output: Path, status: Path | None = None, max_age_hours: float = 168,
        check_docs: bool = False, consume_report: bool = False,
        native_output: Path | None = None, native_platform: str | None = None,
        review_output: Path | None = None, require_ready: bool = False,
        review_json: Path | None = None, consume_review: bool = False,
        decision_output: Path | None = None) -> list[str]:
    ingestion_errors = []
    if native_output is not None:
        if native_platform not in {"windows", "macos", "linux"}:
            ingestion_errors.append(f"unsupported native platform: {native_platform}")
        else:
            target = bundles / native_platform / "manifest.json"
            target.parent.mkdir(parents=True, exist_ok=True)
            ingestion_errors.extend(ingester.ingest(native_output, target))
            try:
                ingested = json.loads(target.read_text(encoding="utf-8"))
                if ingested.get("platform", "").lower() != native_platform:
                    ingestion_errors.append(f"native artifact platform mismatch: {ingested.get('platform')} != {native_platform}")
            except (OSError, json.JSONDecodeError):
                pass
    report = generator.build_report(bundles)
    errors = list(ingestion_errors)
    report["ingestion_errors"] = ingestion_errors
    if status is not None:
        readiness_errors = readiness.check(status, bundles, max_age_hours)
        report["handoff_check_errors"] = readiness_errors
        errors.extend(readiness_errors)
    else:
        report["handoff_check_errors"] = []
    documentation_errors = []
    if check_docs:
        if handoff_docs.main() != 0:
            documentation_errors.append("qualification handoff documentation checker failed")
    report["documentation_check_errors"] = documentation_errors
    errors.extend(documentation_errors)
    errors.extend(validator.validate(report))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if review_output is not None:
        review_output.parent.mkdir(parents=True, exist_ok=True)
        review_output.write_text(reviewer.render(output), encoding="utf-8")
        if require_ready and report.get("exit_gate_status") != "READY":
            errors.append(f"review gate is not ready: {report.get('exit_gate_status')}")
    if review_json is not None:
        review_json.parent.mkdir(parents=True, exist_ok=True)
        review_json.write_text(json.dumps(reviewer.review_packet(output), indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if consume_review or require_ready:
            # Strict handoff consumes the retained pair as one bundle. This
            # prevents a copied/replaced report from being accepted with an
            # otherwise valid review packet.
            errors.extend(bundle_consumer.consume(review_json.parent, output.name, review_json.name))
    if consume_report:
        errors.extend(consumer.consume(output))
    if decision_output is not None:
        if review_json is None:
            errors.append("decision output requires review_json")
        else:
            decision = decision_consumer.decide(review_json.parent, output.name, review_json.name)
            decision_output.parent.mkdir(parents=True, exist_ok=True)
            decision_output.write_text(json.dumps(decision, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            if decision["decision"] != "ACCEPTED":
                errors.append(f"release decision is {decision['decision']}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundles", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--status", type=Path, help="status matrix to check against retained bundles")
    parser.add_argument("--max-age-hours", type=float, default=168)
    parser.add_argument("--check-docs", action="store_true", help="run the qualification documentation/tool checker")
    parser.add_argument("--consume", action="store_true", help="run the downstream report consumer after writing")
    parser.add_argument("--native-output", type=Path, help="native qualification script output to ingest")
    parser.add_argument("--native-platform", choices=("windows", "macos", "linux"))
    parser.add_argument("--review-output", type=Path, help="write a human native-review checklist")
    parser.add_argument("--require-ready", action="store_true", help="fail if exported review is not READY")
    parser.add_argument("--review-json", type=Path, help="write a machine-readable review packet")
    parser.add_argument("--consume-review", action="store_true", help="consume the emitted review JSON packet")
    parser.add_argument("--decision-output", type=Path, help="write a conservative release decision artifact")
    args = parser.parse_args()
    errors = run(args.bundles, args.output, args.status, args.max_age_hours, args.check_docs, args.consume,
                 args.native_output, args.native_platform, args.review_output, args.require_ready, args.review_json,
                 args.consume_review, args.decision_output)
    if errors:
        print("\n".join(errors))
        return 1
    print(f"wrote and validated conservative readiness handoff: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
