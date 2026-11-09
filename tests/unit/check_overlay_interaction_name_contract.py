"""Verify overlay interaction mode names include recovery readiness."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayInteractionMode"',
    'Accessible.name: "Overlay interaction mode: " + text + (window.overlayDisplayRecoveryPending ? ", recovery pending" : ", ready")',
    'Accessible.value: text + (window.overlayDisplayRecoveryPending ? "; recovery pending; settings retained" : "")',
    '"Input mode: click-through"',
    '"Input mode: interactive"',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay interaction name markers: " + ", ".join(missing))
print(f"Overlay interaction name contract verified ({len(required)} checks)")
