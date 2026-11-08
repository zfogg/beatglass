"""Verify a keyboard shortcut focuses persisted recovery selection."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    'objectName: "persistedRecoveryShortcut"',
    'sequence: "Ctrl+Shift+R"',
    "navigation.currentIndex = 0",
    "persistedRecoveryRuns.forceActiveFocus()",
    "Persisted recovery selector focused; choose a saved run to retry or discard.",
]
missing = [marker for marker in required if marker not in qml]
if missing:
    raise SystemExit("Missing persisted recovery shortcut markers: " + ", ".join(missing))
print(f"Recovery selection shortcut contract verified ({len(required)} checks)")
