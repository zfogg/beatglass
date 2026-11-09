"""Verify opacity bridge failures roll the QML slider back safely."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
for marker in ("const applied = window.overlayBridge.setOpacity(value)", "if (!applied)", "value = window.overlayBridge.opacity"):
    if marker not in qml:
        raise SystemExit(f"Missing opacity rollback marker: {marker}")
print("Overlay opacity result contract verified (3 checks)")
