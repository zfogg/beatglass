"""Contract check for explicit persisted recovery retry semantics."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = ["retrySelectedRun", 'objectName: "retryPersistedRecovery"', "re-submit the preserved analysis identity", "selectedRunId !== \"\""]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing recovery retry markers: " + ", ".join(missing))
print(f"Recovery retry contract verified ({len(required)} checks)")
