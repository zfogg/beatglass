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
    summary = b'{"commit":"fixture-commit","qualified":true}\n'
    junit = b"<testsuite tests='1'/>\n"
    artifacts = {"native-summary.json": summary, "pipewire-tests.xml": junit}
    entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in artifacts.items()]
    manifest = {"format": "beatglass-native-pipewire-evidence-v1", "commit": "fixture-commit", "qualified": True, "artifacts": entries}

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
print("PipeWire evidence archive verifier behavior passed (4 scenarios)")
