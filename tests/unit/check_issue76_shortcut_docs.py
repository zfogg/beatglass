from pathlib import Path
root = Path(__file__).resolve().parents[2]
text = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["Shortcut announcement contract", "accessible lifecycle announcement", "preserve focus", "clear stale result", "screen-reader announcement timing"]
missing = [m for m in required if m not in text]
if missing: raise SystemExit("Missing issue #76 shortcut markers: " + ", ".join(missing))
print(f"Issue #76 shortcut documentation verified ({len(required)} checks)")
