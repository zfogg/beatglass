"""Verify successful overlay size changes advance the settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'window.overlayPreferredSizeIndex = requestedIndex\n                                window.overlaySettingsRevision += 1'
if marker not in qml:
    raise SystemExit("Missing overlay size settings revision update")
print("Overlay size revision contract verified (1 check)")
