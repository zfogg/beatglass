"""Verify API documentation build inputs and indexed production headers."""
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import sys


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    required = (root / "docs/api/conf.py", root / "docs/api/index.rst", root / "docs/api/Dockerfile")
    missing = [str(path.relative_to(root)) for path in required if not path.is_file()]
    readme = (root / "README.md").read_text(encoding="utf-8")
    if "sphinx-build -W --keep-going -b html -c docs/api" not in readme:
        missing.append("README.md:sphinx-build -W --keep-going")
    spec = spec_from_file_location("check_headers", root / "docs/api/check_headers.py")
    checker = module_from_spec(spec); assert spec.loader
    spec.loader.exec_module(checker)
    missing_headers, stale = checker.coverage(root)
    repeated = checker.duplicates(root) if hasattr(checker, "duplicates") else []
    if missing_headers or stale or repeated:
        missing.append(f"header index (missing={missing_headers}, stale={stale}, duplicates={repeated})")
    if missing:
        print(f"incomplete API documentation contract: {missing}", file=sys.stderr)
        return 1
    print("API documentation build and header-index contract is complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
