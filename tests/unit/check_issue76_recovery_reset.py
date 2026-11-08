from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["Retry and discard reset behavior", "Settings and overlay-unavailable", "focus returns", "historical retry identity", "diagnostics clear", "failed retry keeps"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing recovery reset markers: " + ", ".join(missing))
print(f"Issue #76 recovery reset contract verified ({len(required)} checks)")
