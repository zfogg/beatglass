"""Verify the summary review action describes pending versus ready recovery."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlaySettingsSummaryReviewAction"',
    'Accessible.description: "Focus the consolidated overlay settings summary for display, size, opacity, and recovery state; "',
    'window.overlayDisplayRecoveryPending ? "recovery is pending; " : "recovery is ready; "',
    '"current settings revision " + window.overlaySettingsRevision',
    'ToolTip.text: Accessible.description',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing summary review description markers: " + ", ".join(missing))
print(f"Overlay summary review description-state contract verified ({len(required)} checks)")
