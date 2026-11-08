"""Verify overlay actions expose launch fallback and hidden-user state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "overlayActions"',
    'window.overlayHiddenByUser ? "available; hidden by user"',
    'window.overlayLaunchFallbackActive',
    'native overlay launch fallback is active',
    'Overlay actions are available; hidden by user',
    'control-window fallback is active',
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing overlay action launch-state markers: " + ", ".join(missing))
print(f"Overlay action launch-state contract verified ({len(required)} checks)")
