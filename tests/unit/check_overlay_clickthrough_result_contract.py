"""Verify the click-through control handles native setter failures."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("const applied = window.overlayBridge.setClickThrough(requestedClickThrough)", "if (!applied || checked !== requestedClickThrough)"):
    if marker not in qml:
        raise SystemExit(f"Missing click-through result marker: {marker}")
print("Overlay click-through result contract verified (2 checks)")
