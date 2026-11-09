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
    summary = (json.dumps({"commit":"fixture-commit", "qualified":True, "native_test_count":1, "native_test_name_match":True, "native_junit_test_count":1, "native_junit_failures":0, "native_junit_errors":0, "native_junit_skipped":0, "native_junit_exists":True, "native_test_discovery":{"count":1,"tests":[{"name":"capture"}]}, "native_discovery_json":"/evidence/pipewire-tests-discovery.json", "native_junit":"/evidence/pipewire-tests.xml", "native_discovery_sha256":hashlib.sha256(discovery).hexdigest(), "native_junit_sha256":hashlib.sha256(junit).hexdigest()}) + "\n").encode()
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

    aggregate = root / "aggregate-count-mismatch.zip"
    aggregate_junit = b"<testsuites tests='2'><testsuite tests='2'><testcase name='capture'/></testsuite></testsuites>\n"
    aggregate_summary = json.loads(summary)
    aggregate_summary["native_junit_test_count"] = 1
    aggregate_summary["native_junit_sha256"] = hashlib.sha256(aggregate_junit).hexdigest()
    aggregate_artifacts = dict(artifacts, **{"native-summary.json": (json.dumps(aggregate_summary) + "\n").encode(), "pipewire-tests.xml": aggregate_junit})
    aggregate_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in aggregate_artifacts.items()]
    aggregate_manifest = dict(manifest, artifacts=aggregate_entries)
    with zipfile.ZipFile(aggregate, "w") as archive:
        for name, data in aggregate_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(aggregate_manifest))
    result = subprocess.run([sys.executable, str(verifier), str(aggregate)], capture_output=True, text=True)
    if result.returncode == 0 or "suite test counts do not match testcases" not in result.stderr + result.stdout: raise SystemExit("aggregate JUnit count mismatch was accepted")

    nested_failure = root / "nested-failure.zip"
    nested_failure_junit = b"<testsuites tests='1' failures='1'><testsuite tests='1' failures='1'><testcase name='capture'/></testsuite></testsuites>\n"
    nested_failure_summary = dict(aggregate_summary, native_junit_sha256=hashlib.sha256(nested_failure_junit).hexdigest())
    nested_failure_artifacts = dict(artifacts, **{"native-summary.json": (json.dumps(nested_failure_summary) + "\n").encode(), "pipewire-tests.xml": nested_failure_junit})
    nested_failure_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in nested_failure_artifacts.items()]
    with zipfile.ZipFile(nested_failure, "w") as archive:
        for name, data in nested_failure_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=nested_failure_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(nested_failure)], capture_output=True, text=True)
    if result.returncode == 0 or "reports failures or errors" not in result.stderr + result.stdout: raise SystemExit("nested JUnit failure count was accepted")

    malformed = root / "malformed-count.zip"
    malformed_junit = b"<testsuite tests='not-a-number'><testcase name='capture'/></testsuite>\n"
    malformed_summary = dict(json.loads(summary), native_junit_sha256=hashlib.sha256(malformed_junit).hexdigest())
    malformed_artifacts = dict(artifacts, **{"native-summary.json": (json.dumps(malformed_summary) + "\n").encode(), "pipewire-tests.xml": malformed_junit})
    malformed_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in malformed_artifacts.items()]
    with zipfile.ZipFile(malformed, "w") as archive:
        for name, data in malformed_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=malformed_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(malformed)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed tests count" not in result.stderr + result.stdout: raise SystemExit("malformed JUnit count was accepted")

    per_suite = root / "per-suite-count.zip"
    per_suite_junit = b"<testsuites tests='2'><testsuite tests='2'><testcase name='capture'/></testsuite><testsuite tests='0'><testcase name='other'/></testsuite></testsuites>\n"
    per_suite_summary = dict(json.loads(summary), native_test_count=2, native_junit_test_count=2, native_junit_sha256=hashlib.sha256(per_suite_junit).hexdigest())
    per_suite_discovery = b'{"tests": [{"name": "capture"}, {"name": "other"}]}\n'
    per_suite_summary["native_discovery_sha256"] = hashlib.sha256(per_suite_discovery).hexdigest()
    per_suite_summary["native_test_discovery"] = {"count": 2, "tests": [{"name": "capture"}, {"name": "other"}]}
    per_suite_artifacts = {"native-summary.json": (json.dumps(per_suite_summary) + "\n").encode(), "pipewire-tests-discovery.json": per_suite_discovery, "pipewire-tests.xml": per_suite_junit}
    per_suite_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in per_suite_artifacts.items()]
    with zipfile.ZipFile(per_suite, "w") as archive:
        for name, data in per_suite_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=per_suite_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(per_suite)], capture_output=True, text=True)
    if result.returncode == 0 or "does not match its testcases" not in result.stderr + result.stdout: raise SystemExit("per-suite JUnit count mismatch was accepted")

    malformed_xml = root / "malformed-xml.zip"
    malformed_payload = b"<testsuite><testcase name='capture'>"
    malformed_xml_summary = dict(json.loads(summary), native_junit_sha256=hashlib.sha256(malformed_payload).hexdigest())
    malformed_xml_artifacts = dict(artifacts, **{"native-summary.json": (json.dumps(malformed_xml_summary) + "\n").encode(), "pipewire-tests.xml": malformed_payload})
    malformed_xml_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in malformed_xml_artifacts.items()]
    with zipfile.ZipFile(malformed_xml, "w") as archive:
        for name, data in malformed_xml_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=malformed_xml_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(malformed_xml)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed XML" not in result.stderr + result.stdout: raise SystemExit("malformed JUnit XML was accepted")

    aggregate_mismatch = root / "aggregate-mismatch.zip"
    aggregate_mismatch_junit = b"<testsuites tests='3'><testsuite tests='1'><testcase name='capture'/></testsuite></testsuites>\n"
    aggregate_mismatch_summary = dict(json.loads(summary), native_junit_sha256=hashlib.sha256(aggregate_mismatch_junit).hexdigest())
    aggregate_mismatch_artifacts = dict(artifacts, **{"native-summary.json": (json.dumps(aggregate_mismatch_summary) + "\n").encode(), "pipewire-tests.xml": aggregate_mismatch_junit})
    aggregate_mismatch_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in aggregate_mismatch_artifacts.items()]
    with zipfile.ZipFile(aggregate_mismatch, "w") as archive:
        for name, data in aggregate_mismatch_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=aggregate_mismatch_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(aggregate_mismatch)], capture_output=True, text=True)
    if result.returncode == 0 or "aggregate test count does not match suites" not in result.stderr + result.stdout: raise SystemExit("aggregate JUnit count mismatch was accepted")
print("PipeWire evidence archive verifier behavior passed (12 scenarios)")
