"""Fail when API header entries are missing, stale, or duplicated."""
from pathlib import Path
import sys
from collections import Counter


def coverage(root: Path) -> tuple[list[str], list[str]]:
    """Return missing and stale index entries, including private lib headers."""
    index = (root / "docs/api/index.rst").read_text(encoding="utf-8")
    listed = set()
    for line in index.splitlines():
        if line.startswith(".. cpp:autodoc:: "):
            listed.update(line.split("::", 1)[1].split())
    headers = {p.relative_to(root).as_posix() for directory in ("include", "lib", "src")
               for p in (root / directory).rglob("*")
               if p.is_file() and p.suffix in (".h", ".hh", ".hpp", ".hxx")}
    return sorted(headers - listed), sorted(listed - headers)


def duplicates(root: Path) -> list[str]:
    """Return headers included more than once by autodoc directives."""
    index = (root / "docs/api/index.rst").read_text(encoding="utf-8")
    entries = (entry for line in index.splitlines() if line.startswith(".. cpp:autodoc:: ")
               for entry in line.split("::", 1)[1].split())
    return sorted(path for path, count in Counter(entries).items() if count > 1)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    missing, extra = coverage(root)
    repeated = duplicates(root)
    if missing or extra or repeated:
        print(f"Missing headers: {missing}; stale entries: {extra}; duplicates: {repeated}", file=sys.stderr)
        return 1
    print("API index covers all production headers")
    return 0


if __name__ == "__main__":
    sys.exit(main())
