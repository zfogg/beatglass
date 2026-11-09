"""Verify an exported native PipeWire evidence archive independently."""
import hashlib
import json
import sys
import zipfile
from datetime import datetime
from pathlib import PurePosixPath

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_native_evidence_archive.py EVIDENCE.zip")
archive_path = sys.argv[1]
with zipfile.ZipFile(archive_path) as archive:
    raw_names = archive.namelist()
    if len(raw_names) != len(set(raw_names)):
        raise SystemExit("duplicate archive entry")
    if any(name.endswith("/") or name.startswith("/") or ".." in PurePosixPath(name).parts for name in raw_names):
        raise SystemExit("unsafe archive entry name")
    names = set(raw_names)
    if "evidence-manifest.json" not in names:
        raise SystemExit("missing evidence-manifest.json")
    manifest = json.loads(archive.read("evidence-manifest.json"))
    if manifest.get("format") != "beatglass-native-pipewire-evidence-v1" or not manifest.get("qualified"):
        raise SystemExit("invalid or unqualified evidence manifest")
    if not isinstance(manifest.get("commit"), str) or not manifest["commit"].strip():
        raise SystemExit("evidence manifest has no commit identity")
    if not isinstance(manifest.get("source_date_epoch"), int) or not manifest.get("archive_timestamp_utc", "").endswith("Z"):
        raise SystemExit("evidence manifest has invalid reproducibility metadata")
    try:
        datetime.fromisoformat(manifest["archive_timestamp_utc"][:-1])
    except ValueError:
        raise SystemExit("evidence manifest has malformed archive timestamp")
    artifacts = manifest.get("artifacts", [])
    if not artifacts:
        raise SystemExit("evidence manifest contains no artifacts")
    if len({artifact.get("name") for artifact in artifacts}) != len(artifacts):
        raise SystemExit("duplicate manifest artifact")
    declared = {artifact["name"] for artifact in artifacts}
    unexpected = names - declared - {"evidence-manifest.json"}
    if unexpected:
        raise SystemExit(f"unlisted archive artifact: {sorted(unexpected)[0]}")
    summary_names = [name for name in declared if name.endswith(".json") and "summary" in name]
    if len(summary_names) != 1 or json.loads(archive.read(summary_names[0])).get("commit") != manifest.get("commit"):
        raise SystemExit("manifest commit does not match qualification summary")
    if not any((name.endswith(".xml") and "junit" in name) or name.endswith("tests.xml") for name in declared):
        raise SystemExit("evidence archive has no JUnit results")
    for artifact in artifacts:
        name = artifact["name"]
        if name not in names:
            raise SystemExit(f"manifest artifact missing from archive: {name}")
        payload = archive.read(name)
        if len(payload) != artifact["size"]:
            raise SystemExit(f"size mismatch: {name}")
        if hashlib.sha256(payload).hexdigest() != artifact["sha256"]:
            raise SystemExit(f"digest mismatch: {name}")
print("Native PipeWire evidence archive verified")
