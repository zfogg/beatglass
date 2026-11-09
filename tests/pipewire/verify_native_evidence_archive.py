"""Verify an exported native PipeWire evidence archive independently."""
import hashlib
import json
import sys
import zipfile
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
    artifacts = manifest.get("artifacts", [])
    if len({artifact.get("name") for artifact in artifacts}) != len(artifacts):
        raise SystemExit("duplicate manifest artifact")
    declared = {artifact["name"] for artifact in artifacts}
    unexpected = names - declared - {"evidence-manifest.json"}
    if unexpected:
        raise SystemExit(f"unlisted archive artifact: {sorted(unexpected)[0]}")
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
