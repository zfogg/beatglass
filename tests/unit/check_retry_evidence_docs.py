from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/progress.md").read_text(encoding="utf-8") + (root / "docs/platform-qualification-status.md").read_text(encoding="utf-8")
required = ["compatible retry", "incompatible retry", "rejected fail-closed", "original and retry commands", "negative case"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing retry evidence markers: " + ", ".join(missing))
print(f"Retry evidence handoff documentation verified ({len(required)} checks)")
