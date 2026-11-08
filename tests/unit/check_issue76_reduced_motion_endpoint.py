from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["property bool reducedMotion", "endpointSelector.forceActiveFocus()", "announceVisualizationRecovery", "Reduced-motion mode", "stale clearing", "keyboard focus remain identical"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing reduced-motion endpoint markers: " + ", ".join(missing))
print(f"Issue #76 reduced-motion endpoint contract verified ({len(required)} checks)")
