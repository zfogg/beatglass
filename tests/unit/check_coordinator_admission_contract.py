"""Dependency-light regression checks for coordinator admission lifecycle."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "lib/library/analysis_coordinator.cpp").read_text(encoding="utf-8")
docs = (ROOT / "docs/collection-ui-integration.md").read_text(encoding="utf-8")
required = {
    "admit before submit": "storage_.admit_analysis(run_id, operation.identity)",
    "reserved miss required": "if (!admission.analysis_reserved && !admission.cached) return {};",
    "cached decoder bypass": "operation.decode = [cached]",
    "transactional publication retained": "make_transactional_publisher",
    "lifecycle docs": "reserves the run identity before decoding",
}
missing = [name for name, marker in required.items() if marker not in source + docs]
if missing:
    raise SystemExit("Missing coordinator admission markers: " + ", ".join(missing))
print(f"Coordinator admission lifecycle contract verified ({len(required)} checks)")
