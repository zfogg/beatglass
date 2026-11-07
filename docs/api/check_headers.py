"""Fail when a production C++ header is absent from the API index."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
index = (root / "docs/api/index.rst").read_text(encoding="utf-8")
listed = set()
for line in index.splitlines():
    if line.startswith(".. cpp:autodoc:: "):
        listed.update(line.split("::", 1)[1].split())
headers = {p.relative_to(root).as_posix() for directory in ("include", "src")
           for p in (root / directory).rglob("*")
           if p.is_file() and p.suffix in (".h", ".hh", ".hpp", ".hxx")}
missing = sorted(headers - listed)
extra = sorted(listed - headers)
if missing or extra:
    print(f"Missing headers: {missing}; stale entries: {extra}", file=sys.stderr)
    sys.exit(1)
print(f"API index covers {len(headers)} production headers")
