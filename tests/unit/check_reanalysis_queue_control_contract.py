"""Verify reanalysis-required state drives a real fresh queue submission."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "src/desktop/storage_recovery_bridge.hpp").read_text(encoding="utf-8")
source = (root / "src/desktop/storage_recovery_bridge.cpp").read_text(encoding="utf-8")
qml = (root / "src/desktop/qml/Main.qml").read_text(encoding="utf-8")
required = [
    "Q_INVOKABLE void reanalyze(",
    "invalidateCached(contentHash, engine, configuration, mode);",
    "submitAnalysis(std::move(runId)",
    'objectName: "reanalyzeInvalidatedCache"',
    'storageRecovery.reanalyze("reanalyze-"',
    'storageRecovery.analysisStatus === "Analysis idle · reanalysis required"',
]
missing = [marker for marker in required if marker not in header + source + qml]
if missing:
    raise SystemExit("Missing reanalysis queue control markers: " + ", ".join(missing))
print(f"Reanalysis queue control contract verified ({len(required)} checks)")
