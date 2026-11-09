"""Verify successful click-through changes advance settings revision."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
marker = 'else\n                                window.overlaySettingsRevision += 1'
if marker not in qml:
    raise SystemExit("Missing click-through settings revision update")
print("Overlay click-through revision contract verified (1 check)")
