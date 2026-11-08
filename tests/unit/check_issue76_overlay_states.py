from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ['objectName: "showOverlay"', 'objectName: "unlockOverlay"', 'objectName: "hideOverlay"', 'objectName: "overlayDisplayEmptyState"', 'objectName: "retryOverlay"', "native overlay bridge", "display-loss"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing overlay state markers: " + ", ".join(missing))
print(f"Issue #76 overlay states contract verified ({len(required)} checks)")
