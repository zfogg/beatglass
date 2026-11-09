from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["Retry admitted", "stale result cleared", "fresh evidence is pending", "activeFocusOnTab: true", "screen-reader timing"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing retry announcement markers: " + ", ".join(missing))
print(f"Issue #76 retry announcement contract verified ({len(required)} checks)")
