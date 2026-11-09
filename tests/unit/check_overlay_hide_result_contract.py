"""Verify overlay hide exposes a native result contract."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
header = (root / "src/desktop/platform/windows_overlay.hpp").read_text(encoding="utf-8")
if "Q_INVOKABLE bool hideOverlay();" not in header:
    raise SystemExit("Missing native hide result contract")
print("Overlay hide result contract verified (1 check)")
