"""Fail when a parser-defined user-facing flag is absent from README.md."""
from pathlib import Path
import re

ROOT = Path(__file__).parents[2]
README = (ROOT / "README.md").read_text(encoding="utf-8")
flags = set()
for path in (ROOT / "tests").rglob("*.py"):
    text = path.read_text(encoding="utf-8", errors="ignore")
    flags.update(re.findall(r'add_argument\(\s*["\'](--[A-Za-z0-9][A-Za-z0-9-]*)', text))
    if path.name in {"qualification_matrix.py", "validate_qualification_artifacts.py"}:
        flags.update(re.findall(r'["\'](--[A-Za-z0-9][A-Za-z0-9-]*)["\']', text))
for path in (ROOT / "src").rglob("*.cpp"):
    text = path.read_text(encoding="utf-8", errors="ignore")
    flags.update(re.findall(r'["\'](--[A-Za-z0-9][A-Za-z0-9-]*)["\']', text))
missing = sorted(flag for flag in flags if flag not in README)
if missing:
    raise SystemExit("README is missing parser flags: " + ", ".join(missing))
print(f"README CLI flag coverage verified ({len(flags)} flags)")
