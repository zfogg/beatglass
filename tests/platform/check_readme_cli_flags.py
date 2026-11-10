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

# Keep the shipped music CLI's short aliases and usage contract visible too.
music_source = (ROOT / "src/cli/music_main.cpp").read_text(encoding="utf-8")
for token in ("--help", "-h", "--version", "-V"):
    if token in music_source and token not in README:
        raise SystemExit(f"README is missing music CLI token: {token}")
if "beatglass-music-cli" in music_source and "beatglass-music-cli" not in README:
    raise SystemExit("README is missing beatglass-music-cli documentation")
print("README music CLI alias coverage verified")
