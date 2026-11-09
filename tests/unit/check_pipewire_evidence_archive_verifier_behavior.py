"""Exercise successful and rejected native evidence archive handoffs."""
import hashlib
import json
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

verifier = Path(__file__).parents[1] / "pipewire" / "verify_native_evidence_archive.py"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    junit = b"<testsuite tests='1'><testcase name='capture'/></testsuite>\n"
    discovery = b'{"tests": [{"name": "capture"}]}\n'
    summary = (json.dumps({"commit":"fixture-commit", "qualified":True, "native_test_count":1, "native_test_name_match":True, "native_junit_test_count":1, "native_junit_failures":0, "native_junit_errors":0, "native_junit_skipped":0, "native_junit_exists":True, "native_test_discovery":{"count":1,"tests":[{"name":"capture"}]}, "native_discovery_sha256":hashlib.sha256(discovery).hexdigest(), "native_junit_sha256":hashlib.sha256(junit).hexdigest()}) + "\n").encode()
    artifacts = {"native-summary.json": summary, "pipewire-tests-discovery.json": discovery, "pipewire-tests.xml": junit}
    entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in artifacts.items()]
    manifest = {"format": "beatglass-native-pipewire-evidence-v1", "commit": "fixture-commit", "qualified": True, "source_date_epoch": 0, "archive_timestamp_utc": "1980-01-01T00:00:00Z", "artifacts": entries}

    good = root / "good.zip"
    with zipfile.ZipFile(good, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(good)], capture_output=True, text=True)
    if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)

    bad = root / "unlisted.zip"
    with zipfile.ZipFile(bad, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("extra.log", b"unlisted")
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(bad)], capture_output=True, text=True)
    if result.returncode == 0 or "unlisted archive artifact" not in result.stderr + result.stdout: raise SystemExit("unlisted artifact was accepted")

    tampered = root / "tampered.zip"
    changed = dict(artifacts)
    changed["pipewire-tests.xml"] = b"<testsuite tests='99'/>\n"
    with zipfile.ZipFile(tampered, "w") as archive:
        for name, data in changed.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(tampered)], capture_output=True, text=True)
    if result.returncode == 0 or not any(message in result.stderr + result.stdout for message in ("size mismatch", "digest mismatch")): raise SystemExit("tampered artifact was accepted")

    duplicate = root / "duplicate.zip"
    with zipfile.ZipFile(duplicate, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("native-summary.json", summary)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(duplicate)], capture_output=True, text=True)
    if result.returncode == 0 or "duplicate archive entry" not in result.stderr + result.stdout: raise SystemExit("duplicate archive entry was accepted")

    traversal = root / "traversal.zip"
    with zipfile.ZipFile(traversal, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("../escape.log", b"unsafe")
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(traversal)], capture_output=True, text=True)
    if result.returncode == 0 or "unsafe archive entry" not in result.stderr + result.stdout: raise SystemExit("unsafe archive entry was accepted")

    empty = root / "empty.zip"
    empty_manifest = dict(manifest, artifacts=[])
    with zipfile.ZipFile(empty, "w") as archive:
        archive.writestr("evidence-manifest.json", json.dumps(empty_manifest))
    result = subprocess.run([sys.executable, str(verifier), str(empty)], capture_output=True, text=True)
    if result.returncode == 0 or "contains no artifacts" not in result.stderr + result.stdout: raise SystemExit("empty manifest was accepted")
print("PipeWire evidence archive verifier behavior passed (6 scenarios)")
