"""Verify switching configured databases closes the previous storage first."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
required = [
    "if (!databasePath_.isEmpty() && !path.isEmpty()) close();",
    "databasePath_ = path; emit databasePathChanged();",
    'status_ = QStringLiteral("Storage closed"); emit statusChanged();',
]
missing = [marker for marker in required if marker not in source]
if missing:
    raise SystemExit("Missing storage path switch close markers: " + ", ".join(missing))
print(f"Storage path switch close contract verified ({len(required)} checks)")
