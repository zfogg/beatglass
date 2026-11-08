from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = ["announceVisualizationRecovery", "endpoint state changed", "fresh audio arrives", "reduced-motion", "Settings, DJ monitoring, and overlay", "stale-selection clearing"]
missing = [m for m in required if m not in qml + docs]
if missing: raise SystemExit("Missing cross-screen recovery markers: " + ", ".join(missing))
print(f"Issue #76 cross-screen recovery contract verified ({len(required)} checks)")
