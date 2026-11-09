"""Verify and package a qualified native PipeWire run for transfer."""
import hashlib
import json
import sys
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
                        ("native_junit", "native_junit_sha256")):
    path = Path(summary[key]).resolve()
    if path.is_symlink() or not path.is_file():
        raise SystemExit(f"missing evidence artifact: {path}")
    if summary.get(digest_key) != digest(path):
        raise SystemExit(f"{key} digest mismatch")
    artifacts.append(path)
for command in summary.get("commands", []):
    for key in ("stdout", "stderr"):
        value = command.get(key)
        if value:
            path = Path(value).resolve()
            if path.is_file() and not path.is_symlink() and path not in artifacts:
                artifacts.append(path)

if output_path.exists() or output_path.is_symlink():
    raise SystemExit(f"refusing to overwrite existing output: {output_path}")
output_path.parent.mkdir(parents=True, exist_ok=True)
try:
    with zipfile.ZipFile(output_path, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in artifacts:
            archive.write(path, arcname=path.name)
except FileExistsError:
    raise SystemExit(f"refusing to overwrite existing output: {output_path}")
print(f"Packaged {len(artifacts)} native evidence files: {output_path}")
