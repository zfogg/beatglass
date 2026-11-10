"""Verify that every user-facing --flag literal in source/helpers is in README."""
from pathlib import Path
import re

root = Path(__file__).parents[2]
readme = (root / "README.md").read_text(encoding="utf-8")
files = [p for pattern in ("*.py", "*.cpp", "*.hpp", "*.cmake", "CMakePresets.json")
         for p in root.rglob(pattern) if ".git" not in p.parts and "build" not in p.parts]
flags = set()
for path in files:
    if path.name == "check_readme_all_flags.py":
        continue
    flags.update(re.findall(r"--[A-Za-z][A-Za-z0-9-]*", path.read_text(encoding="utf-8", errors="ignore")))
missing = sorted(flag for flag in flags if flag not in readme)
if missing:
    raise SystemExit("README is missing repository flags: " + ", ".join(missing))
print(f"README repository flag coverage verified ({len(flags)} flags)")
