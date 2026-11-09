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
    transitions = (json.dumps({"schema":"beatglass-pipewire-transitions-v1", "platform":"Linux-test", "commit":"fixture-commit", "events":[
        {"event":"start", "epoch":1, "serial":"sink-a"},
        {"event":"process", "epoch":1, "sample_count":256},
        {"event":"error", "epoch":1, "error_code":-32, "discontinuity":True},
        {"event":"restart", "epoch":2, "serial":"sink-b", "format":{"sample_rate":48000, "channels":2}}
    ]}) + "\n").encode()
    summary = (json.dumps({"commit":"fixture-commit", "qualified":True, "native_test_count":1, "native_test_name_match":True, "native_junit_test_count":1, "native_junit_failures":0, "native_junit_errors":0, "native_junit_skipped":0, "native_junit_exists":True, "native_test_discovery":{"count":1,"tests":[{"name":"capture"}]}, "native_discovery_json":"/evidence/pipewire-tests-discovery.json", "native_junit":"/evidence/pipewire-tests.xml", "native_transition_json":"/evidence/pipewire-transitions.json", "native_transition_sha256":hashlib.sha256(transitions).hexdigest(), "native_transition_event_count":4, "native_discovery_sha256":hashlib.sha256(discovery).hexdigest(), "native_junit_sha256":hashlib.sha256(junit).hexdigest()}) + "\n").encode()
    artifacts = {"native-summary.json": summary, "pipewire-tests-discovery.json": discovery, "pipewire-tests.xml": junit, "pipewire-transitions.json": transitions}
    entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in artifacts.items()]
    manifest = {"format": "beatglass-native-pipewire-evidence-v1", "commit": "fixture-commit", "qualified": True, "source_date_epoch": 0, "archive_timestamp_utc": "1980-01-01T00:00:00Z", "transition_event_count": 4, "transition_schema": "beatglass-pipewire-transitions-v1", "transition_commit": "fixture-commit", "artifacts": entries}

    good = root / "good.zip"
    with zipfile.ZipFile(good, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(good)], capture_output=True, text=True)
    if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)

    provenance_tampered = root / "provenance-tampered.zip"
    with zipfile.ZipFile(provenance_tampered, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, transition_commit="other-commit")))
    result = subprocess.run([sys.executable, str(verifier), str(provenance_tampered)], capture_output=True, text=True)
    if result.returncode == 0 or "transition commit does not match manifest" not in result.stderr + result.stdout: raise SystemExit("tampered transition provenance was accepted")

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

    case_collision = root / "case-collision.zip"
    with zipfile.ZipFile(case_collision, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("Native-Summary.json", b"collision")
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(case_collision)], capture_output=True, text=True)
    if result.returncode == 0 or "case-insensitive archive entry collision" not in result.stderr + result.stdout: raise SystemExit("case-insensitive collision was accepted")

    traversal = root / "traversal.zip"
    with zipfile.ZipFile(traversal, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("..\\escape\u0001.log", b"unsafe")
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
    per_suite_junit = b"<testsuites tests='2'><testsuite tests='3'><testcase name='capture'/></testsuite><testsuite tests='0'><testcase name='other'/></testsuite></testsuites>\n"
    per_suite_summary = dict(json.loads(summary), native_test_count=2, native_junit_test_count=2, native_junit_sha256=hashlib.sha256(per_suite_junit).hexdigest())
    for key in ("native_transition_json", "native_transition_sha256", "native_transition_event_count"):
        per_suite_summary.pop(key, None)
    per_suite_discovery = b'{"tests": [{"name": "capture"}, {"name": "other"}]}\n'
    per_suite_summary["native_discovery_sha256"] = hashlib.sha256(per_suite_discovery).hexdigest()
    per_suite_summary["native_test_discovery"] = {"count": 2, "tests": [{"name": "capture"}, {"name": "other"}]}
    per_suite_artifacts = {"native-summary.json": (json.dumps(per_suite_summary) + "\n").encode(), "pipewire-tests-discovery.json": per_suite_discovery, "pipewire-tests.xml": per_suite_junit}
    per_suite_entries = [{"name": name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()} for name, data in per_suite_artifacts.items()]
    with zipfile.ZipFile(per_suite, "w") as archive:
        for name, data in per_suite_artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=per_suite_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(per_suite)], capture_output=True, text=True)
    if result.returncode == 0 or not any(message in result.stderr + result.stdout for message in ("suite test counts do not match testcases", "aggregate test count does not match suites")): raise SystemExit("per-suite JUnit count mismatch was accepted")

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
    malformed_manifest = root / "malformed-manifest.zip"
    with zipfile.ZipFile(malformed_manifest, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=[{"name": "nested/path.json"}])))
    result = subprocess.run([sys.executable, str(verifier), str(malformed_manifest)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed manifest artifact name" not in result.stderr + result.stdout: raise SystemExit("malformed manifest artifact name was accepted")
    negative_epoch = root / "negative-epoch.zip"
    with zipfile.ZipFile(negative_epoch, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, source_date_epoch=-1)))
    result = subprocess.run([sys.executable, str(verifier), str(negative_epoch)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid reproducibility metadata" not in result.stderr + result.stdout: raise SystemExit("negative source epoch was accepted")
    invalid_platform = root / "invalid-platform.zip"
    with zipfile.ZipFile(invalid_platform, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, platform=42)))
    result = subprocess.run([sys.executable, str(verifier), str(invalid_platform)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid platform identity" not in result.stderr + result.stdout: raise SystemExit("invalid manifest platform was accepted")
    non_boolean_qualified = root / "non-boolean-qualified.zip"
    with zipfile.ZipFile(non_boolean_qualified, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, qualified=1)))
    result = subprocess.run([sys.executable, str(verifier), str(non_boolean_qualified)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid or unqualified evidence manifest" not in result.stderr + result.stdout: raise SystemExit("non-boolean qualification was accepted")
    casefold_manifest = root / "casefold-manifest.zip"
    duplicate_entries = list(manifest["artifacts"]) + [dict(manifest["artifacts"][0], name=manifest["artifacts"][0]["name"].upper())]
    with zipfile.ZipFile(casefold_manifest, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=duplicate_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(casefold_manifest)], capture_output=True, text=True)
    if result.returncode == 0 or "case-insensitive duplicate manifest artifact" not in result.stderr + result.stdout: raise SystemExit("case-insensitive manifest duplicate was accepted")
    boolean_size = root / "boolean-size.zip"
    boolean_entries = [dict(entry, size=True) if entry["name"] == "native-summary.json" else entry for entry in manifest["artifacts"]]
    with zipfile.ZipFile(boolean_size, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, artifacts=boolean_entries)))
    result = subprocess.run([sys.executable, str(verifier), str(boolean_size)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed artifact metadata" not in result.stderr + result.stdout: raise SystemExit("boolean artifact size was accepted")
    nonstring_timestamp = root / "nonstring-timestamp.zip"
    with zipfile.ZipFile(nonstring_timestamp, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, archive_timestamp_utc=123)))
    result = subprocess.run([sys.executable, str(verifier), str(nonstring_timestamp)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid reproducibility metadata" not in result.stderr + result.stdout: raise SystemExit("non-string archive timestamp was accepted")
    mismatched_timestamp = root / "mismatched-timestamp.zip"
    with zipfile.ZipFile(mismatched_timestamp, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, archive_timestamp_utc="1980-01-01T00:01:00Z")))
    result = subprocess.run([sys.executable, str(verifier), str(mismatched_timestamp)], capture_output=True, text=True)
    if result.returncode == 0 or "archive timestamp does not match source date epoch" not in result.stderr + result.stdout: raise SystemExit("epoch/timestamp drift was accepted")
    unrepresentable_epoch = root / "unrepresentable-epoch.zip"
    with zipfile.ZipFile(unrepresentable_epoch, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, data)
        archive.writestr("evidence-manifest.json", json.dumps(dict(manifest, source_date_epoch=10**30)))
    result = subprocess.run([sys.executable, str(verifier), str(unrepresentable_epoch)], capture_output=True, text=True)
    if result.returncode == 0 or "unrepresentable source date epoch" not in result.stderr + result.stdout: raise SystemExit("unrepresentable epoch was accepted")
    summary_epoch_drift = root / "summary-epoch-drift.zip"
    drifted_summary = json.dumps(dict(json.loads(summary), source_date_epoch=1)).encode()
    with zipfile.ZipFile(summary_epoch_drift, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, drifted_summary if name == "native-summary.json" else data)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(summary_epoch_drift)], capture_output=True, text=True)
    if result.returncode == 0 or "manifest source date epoch does not match qualification summary" not in result.stderr + result.stdout: raise SystemExit("summary epoch drift was accepted")
    invalid_summary_epoch = root / "invalid-summary-epoch.zip"
    invalid_epoch_summary = json.dumps(dict(json.loads(summary), source_date_epoch=True)).encode()
    with zipfile.ZipFile(invalid_summary_epoch, "w") as archive:
        for name, data in artifacts.items(): archive.writestr(name, invalid_epoch_summary if name == "native-summary.json" else data)
        archive.writestr("evidence-manifest.json", json.dumps(manifest))
    result = subprocess.run([sys.executable, str(verifier), str(invalid_summary_epoch)], capture_output=True, text=True)
    if result.returncode == 0 or "qualification summary has invalid source date epoch" not in result.stderr + result.stdout: raise SystemExit("invalid summary epoch was accepted")
    malformed_json = root / "malformed-manifest-json.zip"
    with zipfile.ZipFile(malformed_json, "w") as archive:
        archive.writestr("evidence-manifest.json", b"{")
    result = subprocess.run([sys.executable, str(verifier), str(malformed_json)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed evidence manifest" not in result.stderr + result.stdout: raise SystemExit("malformed manifest JSON was accepted")
print("PipeWire evidence archive verifier behavior passed (26 scenarios)")
