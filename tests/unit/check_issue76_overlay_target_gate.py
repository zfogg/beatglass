from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["screenNames.length > 0", "waiting for a display", "reduced-motion", "display-loss", "selected display", "Retry showing"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing overlay target gate markers: " + ", ".join(missing))
print(f"Issue #76 overlay target gate contract verified ({len(required)} checks)")
