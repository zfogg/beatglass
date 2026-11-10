"""Keep portable CLI evidence explicitly separate from native qualification."""
from pathlib import Path

root = Path(__file__).parents[2]
status = (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ("portable CLI/library evidence", "do not advance any native", "Remaining CLI/library gates")
missing = [text for text in required if text not in status]
if missing:
    raise SystemExit("platform status is missing CLI evidence boundary: " + ", ".join(missing))
print("CLI evidence boundary verified")
