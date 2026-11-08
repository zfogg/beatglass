"""Verify the checked-in API documentation build contract is complete."""
from pathlib import Path
import sys


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    required = (root / "docs/api/conf.py", root / "docs/api/index.rst", root / "docs/api/Dockerfile")
    missing = [str(path.relative_to(root)) for path in required if not path.is_file()]
    readme = (root / "README.md").read_text(encoding="utf-8")
    if "sphinx-build -W --keep-going -b html docs/api" not in readme:
        missing.append("README.md:sphinx-build -W --keep-going")
    if missing:
        print(f"missing API documentation contract entries: {missing}", file=sys.stderr)
        return 1
    print("API documentation build contract is present")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
