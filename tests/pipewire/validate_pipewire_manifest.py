"""Validate the shape of a captured PipeWire qualification manifest."""
import json
import hashlib
import re
import sys
import sys
from datetime import datetime
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from manifest_contract import audio_sink_count, inventory_digest

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_pipewire_manifest.py MANIFEST")
manifest_path = Path(sys.argv[1])
if manifest_path.is_symlink():
    raise SystemExit("refusing symlinked manifest")
try:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError) as exc:
    raise SystemExit(f"invalid manifest: {exc}")
required = ("schema_version", "captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory")
if not isinstance(manifest, dict) or any(key not in manifest or manifest[key] is None for key in required):
    raise SystemExit("manifest missing required qualification identity or inventory")
if type(manifest["schema_version"]) is not int or manifest["schema_version"] != 1:
    raise SystemExit("unsupported PipeWire qualification manifest schema")
for field in ("host_platform", "python_version", "pipewire_cli_version"):
    if not isinstance(manifest[field], str) or not manifest[field].strip():
        raise SystemExit(f"manifest {field} must be a non-empty string")
if type(manifest.get("audio_sink_count")) is not int or manifest["audio_sink_count"] < 1:
    raise SystemExit("manifest audio_sink_count must be a positive integer")
if not isinstance(manifest["captured_at_utc"], str) or not manifest["captured_at_utc"].strip():
    raise SystemExit("manifest captured_at_utc must be a non-empty string")
if not str(manifest["host_platform"]).startswith("Linux"):
    raise SystemExit("manifest host_platform must identify Linux")
try:
    captured_at = datetime.fromisoformat(manifest["captured_at_utc"])
except ValueError:
    raise SystemExit("manifest captured_at_utc must be ISO-8601")
if captured_at.utcoffset() is None:
    raise SystemExit("manifest captured_at_utc must include a timezone")
if captured_at.utcoffset().total_seconds() != 0:
    raise SystemExit("manifest captured_at_utc must be UTC")
if not isinstance(manifest["inventory"], list):
    raise SystemExit("manifest inventory must be a list")
if not manifest["inventory"]:
    raise SystemExit("manifest inventory must not be empty")
if any(not isinstance(item, dict) for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must be objects")
if any(not isinstance(item.get("info"), dict) for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must retain info objects")
if any(not isinstance(item["info"].get("props"), dict) for item in manifest["inventory"]):
    raise SystemExit("manifest inventory info must retain props objects")
if not any(item["info"]["props"].get("media.class") == "Audio/Sink" for item in manifest["inventory"]):
    raise SystemExit("manifest inventory must include an Audio/Sink")
if manifest["audio_sink_count"] != audio_sink_count(manifest["inventory"]):
    raise SystemExit("manifest audio_sink_count does not match inventory")
expected_digest = inventory_digest(manifest["inventory"])
if not isinstance(manifest.get("inventory_sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", manifest["inventory_sha256"]):
    raise SystemExit("manifest inventory_sha256 must be 64 lowercase hex characters")
if manifest.get("inventory_sha256") != expected_digest:
    raise SystemExit("manifest inventory_sha256 does not match inventory")
for item in manifest["inventory"]:
    props = item["info"]["props"]
    if "media.class" in props and not isinstance(props["media.class"], str):
        raise SystemExit("manifest media.class must be a string")
    if props.get("media.class") == "Audio/Sink" and (not str(props.get("object.serial", "")).strip() or not str(props.get("node.name", "")).strip()):
        raise SystemExit("Audio/Sink must retain serial and node name")
    if props.get("media.class") == "Audio/Sink" and (not isinstance(props.get("object.serial"), str) or not isinstance(props.get("node.name"), str)):
        raise SystemExit("Audio/Sink serial and node name must be strings")
    if props.get("media.class") == "Audio/Sink" and not re.fullmatch(r"[0-9]+", props["object.serial"]):
        raise SystemExit("Audio/Sink object.serial must be numeric")
sinks = [item["info"]["props"] for item in manifest["inventory"] if item["info"]["props"].get("media.class") == "Audio/Sink"]
if len({p["object.serial"] for p in sinks}) != len(sinks) or len({p["node.name"] for p in sinks}) != len(sinks):
    raise SystemExit("Audio/Sink serials and node names must be unique")
if any(not isinstance(item.get("id"), int) or item["id"] < 0 for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must have non-negative numeric ids")
ids = [item["id"] for item in manifest["inventory"]]
if len(ids) != len(set(ids)):
    raise SystemExit("manifest inventory object ids must be unique")
print("PipeWire qualification manifest is valid")
