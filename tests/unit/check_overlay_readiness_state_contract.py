"""Verify overlay estimate readiness exposes snapshot and recovery state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayDataReadiness"',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; snapshot retained" : "; recovery ready")',
    '"Overlay estimates: accepted snapshot ready"',
    '"Overlay estimates: waiting for accepted snapshot"',
    'The accepted snapshot remains retained while recovery is pending.',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay readiness state markers: " + ", ".join(missing))
print(f"Overlay readiness state contract verified ({len(required)} checks)")
