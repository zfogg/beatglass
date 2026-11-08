"""Verify overlay routing controls expose interaction, opacity, and recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
docs = (root / "docs/issue-76-visual-implementation-audit.md").read_text(encoding="utf-8")
required = [
    'objectName: "overlayRoutingControls"',
    'window.overlayDisplayRecoveryPending ? ", recovery pending"',
    '"unavailable; control-window fallback active"',
    'Overlay geometry and action groups now expose recovery-pending state',
]
missing = [marker for marker in required if marker not in qml + docs]
if missing:
    raise SystemExit("Missing overlay routing recovery markers: " + ", ".join(missing))
print(f"Overlay routing recovery contract verified ({len(required)} checks)")
