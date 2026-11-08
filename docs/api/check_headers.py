"""Fail when a production C++ header is absent from the API index."""
from pathlib import Path
import sys


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
    """Return headers indexed more than once."""
    index = (root / "docs/api/index.rst").read_text(encoding="utf-8")
    entries = [item for line in index.splitlines() if line.startswith(".. cpp:autodoc:: ")
               for item in line.split("::", 1)[1].split()]
    return sorted({item for item in entries if entries.count(item) > 1})


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    missing, extra = coverage(root)
    repeated = duplicates(root)
    if missing or extra or repeated:
        print(f"Missing headers: {missing}; stale entries: {extra}; duplicate entries: {repeated}", file=sys.stderr)
        return 1
    print("API index covers all production headers")
    return 0


if __name__ == "__main__":
    sys.exit(main())
