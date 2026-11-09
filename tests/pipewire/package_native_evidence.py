"""Verify and package a qualified native PipeWire run for transfer."""
import hashlib
import json
import os
import sys
import subprocess
from datetime import datetime, timezone
import zipfile
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


if len(sys.argv) != 3:
    raise SystemExit("usage: package_native_evidence.py SUMMARY.json OUTPUT.zip")
summary_path = Path(sys.argv[1]).resolve()
output_path = Path(sys.argv[2]).resolve()
summary = json.loads(summary_path.read_text(encoding="utf-8"))
if not summary.get("qualified"):
    raise SystemExit("native qualification summary is not qualified")
if summary.get("failed_command") is not None:
    raise SystemExit("native qualification has a failed command")

artifacts = [summary_path]
for key, digest_key in (("native_discovery_json", "native_discovery_sha256"),
                        ("native_junit", "native_junit_sha256"),
                        ("native_transition_json", "native_transition_sha256")):
    if key not in summary:
        continue
    path = Path(summary[key]).resolve()
    if path.is_symlink() or not path.is_file():
        raise SystemExit(f"missing evidence artifact: {path}")
    if summary.get(digest_key) != digest(path):
        raise SystemExit(f"{key} digest mismatch")
    if key == "native_transition_json":
        validator = Path(__file__).with_name("validate_pipewire_transition_evidence.py")
        check = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True, check=False)
        if check.returncode:
            raise SystemExit(check.stderr or check.stdout or "transition evidence validation failed")
    artifacts.append(path)
for command in summary.get("commands", []):
    for key in ("stdout", "stderr"):
        value = command.get(key)
        if value:
            path = Path(value).resolve()
            if path.is_file() and not path.is_symlink() and path not in artifacts:
                artifacts.append(path)
if len({path.name for path in artifacts}) != len(artifacts):
    raise SystemExit("evidence artifacts have colliding archive names")
if len({path.name.casefold() for path in artifacts}) != len(artifacts):
    raise SystemExit("evidence artifacts have case-insensitive archive name collisions")
if any(any(ord(char) < 32 for char in path.name) for path in artifacts):
    raise SystemExit("evidence artifacts have control characters in archive names")

if output_path.exists() or output_path.is_symlink():
    raise SystemExit(f"refusing to overwrite existing output: {output_path}")
output_path.parent.mkdir(parents=True, exist_ok=True)
epoch = max(0, int(os.environ.get("SOURCE_DATE_EPOCH", "0")))
archive_time = datetime.fromtimestamp(epoch, timezone.utc).replace(tzinfo=None)
archive_time = max(archive_time, datetime(1980, 1, 1))
try:
    with zipfile.ZipFile(output_path, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(artifacts, key=lambda item: item.name):
            info = zipfile.ZipInfo(path.name, archive_time.timetuple()[:6])
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, path.read_bytes())
        manifest = {
            "format": "beatglass-native-pipewire-evidence-v1",
            "commit": summary.get("commit"),
            "platform": summary.get("platform"),
            "qualified": True,
            "source_date_epoch": epoch,
            "archive_timestamp_utc": archive_time.isoformat() + "Z",
            "transition_event_count": summary.get("native_transition_event_count"),
            "artifacts": [
                {"name": path.name, "size": path.stat().st_size, "sha256": digest(path)}
                for path in artifacts
            ],
        }
        info = zipfile.ZipInfo("evidence-manifest.json", archive_time.timetuple()[:6])
        info.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(info, json.dumps(manifest, indent=2, sort_keys=True) + "\n")
except FileExistsError:
    raise SystemExit(f"refusing to overwrite existing output: {output_path}")
print(f"Packaged {len(artifacts)} native evidence files: {output_path}")
