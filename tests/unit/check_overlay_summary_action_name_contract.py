"""Verify summary review action names recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsSummaryReviewAction"',
    'Accessible.name: text + ", " + (window.overlayDisplayRecoveryPending ? "recovery pending" : "ready")',
    'Accessible.value: (window.overlayDisplayRecoveryPending ? "recovery pending" : "available") + "; settings revision " + window.overlaySettingsRevision',
    'text: "Review overlay summary"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing summary action name markers: " + ", ".join(missing))
print(f"Overlay summary action name contract verified ({len(required)} checks)")
