"""Verify overlay estimate attribution exposes source readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayEstimateAttribution"',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; source retained" : "; source ready")',
    '"Estimate source: selected system output · combined mix"',
    'The selected combined-output source is retained while recovery is pending.',
    'ToolTip.text: visible ? Accessible.description : "Overlay estimate source"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay attribution state markers: " + ", ".join(missing))
print(f"Overlay attribution state contract verified ({len(required)} checks)")
