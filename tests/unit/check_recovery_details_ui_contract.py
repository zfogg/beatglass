"""Contract check for rendering persisted retry identity in recovery UI."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
bridge = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
required = ["retryableRunDetails", 'objectName: "persistedRecoveryRunDetails"', "Discard saved run", "discardSelectedRun"]
missing = [marker for marker in required if marker not in qml + bridge]
if missing:
    raise SystemExit("Missing recovery details UI markers: " + ", ".join(missing))
print(f"Recovery details UI contract verified ({len(required)} checks)")
