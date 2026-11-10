"""Ensure the progress ledger preserves the CLI evidence boundary."""
from pathlib import Path

text = (Path(__file__).parents[2] / "docs/progress.md").read_text(encoding="utf-8")
for phrase in ("CLI/library evidence update", "portable preparation evidence", "remain open", "no headless result"):
    if phrase not in text:
        raise SystemExit(f"progress ledger is missing CLI evidence boundary: {phrase}")
print("progress CLI evidence boundary verified")
