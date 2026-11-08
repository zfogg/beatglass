from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["overlay display disappeared", "controls restored", 'objectName: "retryOverlay"', 'objectName: "overlayUnavailable"', "monitor loss/recovery", "control window remaining available"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing display-loss markers: " + ", ".join(missing))
print(f"Issue #76 display-loss contract verified ({len(required)} checks)")
