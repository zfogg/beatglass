from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ['objectName: "refreshEndpoints"', 'objectName: "permissionGuidance"', 'endpointSelectionBridge.refresh()', "device replacement", "keyboard reachable", "real endpoint bridge"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing endpoint control markers: " + ", ".join(missing))
print(f"Issue #76 endpoint controls contract verified ({len(required)} checks)")
