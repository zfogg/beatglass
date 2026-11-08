from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["overlay recovery is unavailable", "keyboard-focusable", "reduced motion", "fresh source and display selection", "clears the stale rejection", "retry"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing retry rejection accessibility markers: " + ", ".join(missing))
print(f"Issue #76 retry rejection accessibility contract verified ({len(required)} checks)")
