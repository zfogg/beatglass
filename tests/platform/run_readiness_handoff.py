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


def run(bundles: Path, output: Path, status: Path | None = None, max_age_hours: float = 168) -> list[str]:
    report = generator.build_report(bundles)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    errors = validator.validate(report)
    if status is not None:
        errors.extend(readiness.check(status, bundles, max_age_hours))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundles", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--status", type=Path, help="status matrix to check against retained bundles")
    parser.add_argument("--max-age-hours", type=float, default=168)
    args = parser.parse_args()
    errors = run(args.bundles, args.output, args.status, args.max_age_hours)
    if errors:
        print("\n".join(errors))
        return 1
    print(f"wrote and validated conservative readiness handoff: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
