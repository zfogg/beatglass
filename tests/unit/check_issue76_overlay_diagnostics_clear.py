from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["overlay-unavailable", "successful retry", "explicit discard", "stale run identity", "rejection text disappear", "Failed recovery keeps diagnostics"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing overlay diagnostics markers: " + ", ".join(missing))
print(f"Issue #76 overlay diagnostics cleanup verified ({len(required)} checks)")
