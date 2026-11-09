"""Contract checks for the auditable native evidence handoff packager."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "package_native_evidence.py").read_text(encoding="utf-8")
markers = (
    "zipfile",
    "qualified",
    "failed_command",
    "native_discovery_json",
    "native_junit",
    "native_discovery_sha256",
    "native_junit_sha256",
    "refusing to overwrite",
    "evidence-manifest.json",
    "sha256",
    "beatglass-native-pipewire-evidence-v1",
    "SOURCE_DATE_EPOCH",
    "sorted(artifacts",
    "ZipInfo",
    "source_date_epoch",
    "archive_timestamp_utc",
    "colliding archive names",
    "native_transition_json",
    "validate_pipewire_transition_evidence.py",
    "transition evidence validation failed",
    "transition_event_count",
    "platform",
    "transition_schema",
)
missing = [marker for marker in markers if marker not in source]
if missing:
    raise SystemExit(f"missing evidence-packager contract markers: {missing}")
print(f"PipeWire evidence packager contract passed ({len(markers)} checks)")
