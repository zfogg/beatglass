from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ['objectName: "analysisRecoveryTransition"', "previous source labels are not current", "Source replacement", "endpoint switch", "discontinuity", "focus remains"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing transition status markers: " + ", ".join(missing))
print(f"Issue #76 transition status contract verified ({len(required)} checks)")
